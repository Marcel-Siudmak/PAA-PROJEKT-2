"""
train.py — Trening NNUE metodą TD-Leaf(λ)

Architektura sieci: 768 → 128 (ReLU) → 32 (ReLU) → 1 (tanh)
Dane wejściowe: games.txt wygenerowany przez selfplay_gen

Użycie:
  # Trening od zera:
  python3 train.py --input games.txt --output nn.bin

  # Dotrenowanie (fine-tuning) z istniejącego nn.bin:
  python3 train.py --input games.txt --output nn.bin --resume nn.bin --lr 0.0002

Dodatkowe flagi:
  --epochs N    Liczba epok (domyślnie 150)
  --batch  N    Rozmiar batcha (domyślnie 1024)
  --lr     F    Learning rate (domyślnie 1e-3, przy --resume zalecane 2e-4)
  --resume path Wczytaj wagi z binarnego nn.bin przed treningiem (fine-tuning)
"""

import argparse
import os
import sys

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset

try:
    from board_encoder import fen_to_features, FEATURES_SIZE
except ImportError:
    sys.path.insert(0, os.path.dirname(__file__))
    from board_encoder import fen_to_features, FEATURES_SIZE


# ── Rozmiary warstw (muszą być spójne z nnue.hpp) ─────────────────────────
IN, H1, H2, OUT = 768, 128, 32, 1


# ── Model NNUE ─────────────────────────────────────────────────────────────

class NNUEModel(nn.Module):
    """768 → 128 (ReLU) → 32 (ReLU) → 1 (tanh)"""

    def __init__(self):
        super().__init__()
        self.l1 = nn.Linear(IN,  H1)
        self.l2 = nn.Linear(H1, H2)
        self.l3 = nn.Linear(H2, OUT)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = torch.relu(self.l1(x))
        x = torch.relu(self.l2(x))
        return torch.tanh(self.l3(x))   # ∈ (-1, +1)

    def init_weights(self):
        for layer in [self.l1, self.l2, self.l3]:
            nn.init.xavier_uniform_(layer.weight, gain=0.5)
            nn.init.zeros_(layer.bias)


# ── Wczytywanie wag z binarnego nn.bin → PyTorch ──────────────────────────

def load_binary_weights(model: NNUEModel, path: str) -> bool:
    """
    Wczytuje wagi zapisane przez export_weights() z powrotem do modelu PyTorch.
    Format: float32 little-endian: w1(H1×IN), b1(H1), w2(H2×H1), b2(H2), w3(OUT×H2), b3(OUT)
    """
    expected_bytes = 4 * (H1*IN + H1 + H2*H1 + H2 + OUT*H2 + OUT)
    if not os.path.exists(path):
        return False
    if os.path.getsize(path) != expected_bytes:
        print(f"[WARN] Rozmiar {path} ({os.path.getsize(path)}B) != oczekiwany ({expected_bytes}B) — pomijam resume")
        return False

    data = np.frombuffer(open(path, "rb").read(), dtype=np.float32)
    offset = 0

    def rd(shape):
        nonlocal offset
        n = int(np.prod(shape))
        arr = data[offset:offset + n].reshape(shape).copy()
        offset += n
        return torch.from_numpy(arr)

    with torch.no_grad():
        model.l1.weight.copy_(rd((H1, IN)))
        model.l1.bias.copy_(rd((H1,)))
        model.l2.weight.copy_(rd((H2, H1)))
        model.l2.bias.copy_(rd((H2,)))
        model.l3.weight.copy_(rd((OUT, H2)))
        model.l3.bias.copy_(rd((OUT,)))

    print(f"[resume] Wczytano wagi z: {path}")
    return True


# ── Eksport wag do binarnego pliku C++ ────────────────────────────────────

def export_weights(model: NNUEModel, path: str) -> None:
    """
    Zapisuje wagi w formacie zgodnym z nnue.cpp::load():
      float32 little-endian: w1, b1, w2, b2, w3, b3
    """
    with open(path, "wb") as f:
        for layer in [model.l1, model.l2, model.l3]:
            w = layer.weight.detach().cpu().numpy().astype(np.float32)
            b = layer.bias.detach().cpu().numpy().astype(np.float32)
            f.write(w.tobytes())
            f.write(b.tobytes())
    print(f"  → Wagi zapisane: {path} ({os.path.getsize(path) / 1024:.1f} KB)")


# ── Dataset ────────────────────────────────────────────────────────────────

class GamesDataset(Dataset):
    """
    Czyta games.txt (format: <FEN> | <wynik>)
    Wynik: 1=wygrana strony toczącej ruch, -1=przegrana, 0=remis
    """

    def __init__(self, paths: list[str], max_samples: int = 5_000_000):
        fens, targets = [], []
        errors = 0

        for path in paths:
            print(f"Wczytuję: {path}")
            with open(path, encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        fen_part, result_part = line.rsplit("|", 1)
                        fens.append(fen_part.strip())
                        targets.append(float(int(result_part.strip())))
                    except Exception:
                        errors += 1
                    if len(fens) >= max_samples:
                        break

        print(f"  Łącznie: {len(fens):,} pozycji (błędy: {errors})")
        print(f"  Konwertuję FEN → cechy (może potrwać kilka minut)...")

        X = np.stack([fen_to_features(f) for f in fens], axis=0)
        y = np.array(targets, dtype=np.float32)

        self.X = torch.from_numpy(X)
        self.y = torch.from_numpy(y)

    def __len__(self) -> int:
        return len(self.y)

    def __getitem__(self, idx):
        return self.X[idx], self.y[idx]


# ── Trening ────────────────────────────────────────────────────────────────

def train(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Urządzenie: {device}")

    # Wczytaj dane (obsługa wielu plików oddzielonych przecinkiem)
    input_files = [p.strip() for p in args.input.split(",") if p.strip()]
    dataset = GamesDataset(input_files)
    loader  = DataLoader(dataset, batch_size=args.batch, shuffle=True,
                         num_workers=0, pin_memory=(device.type == "cuda"))

    model = NNUEModel().to(device)

    # Resume — dotrenowanie z istniejących wag
    resumed = False
    if args.resume:
        resumed = load_binary_weights(model, args.resume)
        if not resumed:
            print("[WARN] Nie udało się wczytać wag do resume — trening od zera")
            model.init_weights()
    else:
        model.init_weights()

    optimizer = optim.Adam(model.parameters(), lr=args.lr, weight_decay=1e-5)
    # Ciepły restart lr z cosine annealing
    scheduler = optim.lr_scheduler.CosineAnnealingWarmRestarts(
        optimizer, T_0=max(args.epochs // 3, 10), T_mult=1, eta_min=1e-6
    )
    criterion = nn.MSELoss()

    mode_str = f"DOTRENOWANIE z {args.resume}" if resumed else "TRENING OD ZERA"
    print(f"\n{'='*55}")
    print(f"  {mode_str}")
    print(f"  Epoki: {args.epochs} | Batch: {args.batch} | LR: {args.lr}")
    print(f"  Parametry sieci: {sum(p.numel() for p in model.parameters()):,}")
    print(f"  Pozycji: {len(dataset):,} | Batche/epoka: {len(loader):,}")
    print(f"{'='*55}\n")

    best_loss = float("inf")

    for epoch in range(1, args.epochs + 1):
        model.train()
        total_loss = 0.0
        n_batches  = 0

        for X, y in loader:
            X = X.to(device)
            y = y.to(device).unsqueeze(1)

            pred = model(X)
            loss = criterion(pred, y)

            optimizer.zero_grad()
            loss.backward()
            nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
            optimizer.step()

            total_loss += loss.item()
            n_batches  += 1

        scheduler.step()
        avg_loss = total_loss / max(n_batches, 1)
        lr_now   = optimizer.param_groups[0]["lr"]

        marker = " ★ BEST" if avg_loss < best_loss else ""
        print(f"Epoka {epoch:4d}/{args.epochs} | loss: {avg_loss:.6f} | lr: {lr_now:.7f}{marker}")

        if avg_loss < best_loss:
            best_loss = avg_loss
            export_weights(model, args.output)

    print(f"\n{'='*55}")
    print(f"  Trening zakończony! Najlepszy loss: {best_loss:.6f}")
    print(f"  Wagi NNUE zapisane do: {args.output}")
    print(f"{'='*55}")


# ── CLI ────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="NNUE Trainer — TD-Leaf(λ) style")
    parser.add_argument("--input",  default="games.txt",
                        help="Plik(i) z grami, oddzielone przecinkiem (np. games.txt,games2.txt)")
    parser.add_argument("--output", default="../nn.bin",
                        help="Plik wyjściowy z wagami")
    parser.add_argument("--resume", default="",
                        help="Ścieżka do nn.bin do dotrenowania (fine-tuning)")
    parser.add_argument("--epochs", type=int,   default=150,
                        help="Liczba epok")
    parser.add_argument("--batch",  type=int,   default=1024,
                        help="Rozmiar batcha")
    parser.add_argument("--lr",     type=float, default=3e-4,
                        help="Learning rate (przy resume zalecane 2e-4)")
    args = parser.parse_args()

    missing = [p.strip() for p in args.input.split(",")
               if p.strip() and not os.path.exists(p.strip())]
    if missing:
        print(f"BŁĄD: Nie znaleziono pliku/ów: {missing}")
        print("Wygeneruj dane: ./selfplay_gen --games 2000 --depth 5 --output ml/games2.txt")
        sys.exit(1)

    train(args)
