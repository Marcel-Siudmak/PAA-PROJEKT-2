# CustomChessAI: MCTS + Reinforcement Learning Engine

## Cel projektu
Stworzenie w pełni funkcjonalnego środowiska szachowego oraz wytrenowanie agenta grającego, który zamiast prostych heurystyk (punktacja figur), wykorzystuje głęboką sieć neuronową do oceny pozycji i przeszukiwania drzewa stanów (MCTS).


---

## Architektura Systemu
Projekt podzielony jest na trzy główne moduły:

### 1. Chess Core (Logika Własna)
Implementacja zasad gry bez użycia zewnętrznych bibliotek szachowych.
*   **Bitboard Representation:** Wykorzystanie 64-bitowych liczb całkowitych do reprezentacji położeń figur (wysoka wydajność).
*   **Move Generator:** Autorski algorytm generujący legalne ruchy, uwzględniający:
    *   Roszady i bicie w przelocie (en passant).
    *   Weryfikację szacha i związania (pins).
    *   Promocję pionów.
*   **GameState Manager:** Obsługa historii ruchów, zasady 50 ruchów oraz trzykrotnego powtórzenia pozycji.

### 2. Brain (Sieć Neuronowa - PyTorch)
Model typu *Actor-Critic* oceniający stan planszy.
*   **Input:** Tensor 8x8x12 (pozycje 6 typów figur dla obu kolorów).
*   **Policy Head:** Zwraca rozkład prawdopodobieństwa dla wszystkich możliwych ruchów.
*   **Value Head:** Zwraca skalarną wartość [-1, 1] oceniającą szansę na wygraną z danej pozycji.

### 3. Search Engine (MCTS)
Algorytm przeszukiwania drzewa stanów Monte Carlo, który integruje logikę szachową z przewidywaniami sieci neuronowej.
*   **Selection:** Wybór ścieżki za pomocą wzoru PUCT (Predictive Upper Confidence Bound applied to Trees).
*   **Expansion & Evaluation:** Rozszerzanie drzewa o nowe stany ocenione przez model.
*   **Backup:** Aktualizacja statystyk węzłów (liczba wizyt, średnia wartość) w górę drzewa.

---

## Technologie
*   **Język:** Python 3.10+
*   **Deep Learning:** PyTorch (obsługa CUDA dla szybszego treningu)
*   **GUI:** Pygame (interfejs dla użytkownika końcowego)
*   **Obliczenia:** NumPy (wsparcie dla operacji na macierzach)

---

## Plan Implementacji

1.  **Szkielet ChessCore:** Implementacja klas `Board`, `Piece` oraz generatora ruchów. Testowanie poprawności za pomocą testów typu *Perft* (liczenie węzłów na danej głębokości).
2.  **MCTS Basic:** Implementacja drzewa przeszukiwań z losowymi symulacjami (bez sieci).
3.  **Integracja PyTorch:** Stworzenie architektury sieci i klasy `Wrapper`, która przygotowuje dane z planszy dla modelu.
4.  **Pętla Self-Play:** Uruchomienie procesu, w którym agent gra sam ze sobą, generując dane treningowe.
5.  **Trening:** Cykliczne douczanie modelu na zebranych partiach.
6.  **Interfejs Graficzny:** Stworzenie okna gry, w którym użytkownik może zmierzyć się z wytrenowanym modelem.

---