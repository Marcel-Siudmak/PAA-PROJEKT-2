/*
 * chess_gui.cpp – natywne okno wxWidgets dla silnika szachowego
 *
 * Uwaga: struct Move z silnika koliduje z wxWindow::Move().
 * Rozwiązanie: aliasujemy chess::Move do ChessMove zanim wciągniemy wxWidgets.
 */

// ── najpierw nagłówki silnika (bez wxWidgets) ───────────────────────────────
#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "search.hpp"
#include "types.hpp"
#include "zobrist.hpp"
#include "magic.hpp"

// Alias który pozwala nam używać ChessMove zamiast Move wewnątrz klas wx
using ChessMove = Move;

// ── potem wxWidgets ──────────────────────────────────────────────────────────
#include <wx/wx.h>

#include <vector>
#include <sstream>
#include <string>

// ==============================
// STAŁE KONFIGURACYJNE
// ==============================
static constexpr int SQ   = 70;           // rozmiar pola w px
static constexpr int BSIZ = SQ * 8;       // rozmiar planszy
static constexpr int MAR  = 30;           // margines
static constexpr int PW   = BSIZ + MAR*2; // szerokość panelu planszy
static constexpr int PH   = BSIZ + MAR*2; // wysokość panelu planszy
static constexpr int SW   = 280;           // szerokość panelu bocznego

static const wxColour C_LIGHT (240, 217, 181);
static const wxColour C_DARK  (181, 136,  99);
static const wxColour C_SEL   (124,  58, 237);
static const wxColour C_HINT  (252, 196,  25);
static const wxColour C_BG    ( 24,  24,  32);
static const wxColour C_PANEL ( 34,  34,  48);

// ==============================
// POMOCNICZE
// ==============================
static std::string sqNote(int sq) {
    return std::string(1, char('a' + sq % 8)) + std::to_string(sq / 8 + 1);
}
static std::string mv2str(const ChessMove &m) {
    std::string pr;
    if (m.promotion != PieceType::NONE)
        switch (m.promotion) {
            case PieceType::QUEEN:  pr = "q"; break;
            case PieceType::ROOK:   pr = "r"; break;
            case PieceType::BISHOP: pr = "b"; break;
            case PieceType::KNIGHT: pr = "n"; break;
            default: break;
        }
    return sqNote(m.from) + sqNote(m.to) + pr;
}

// ==============================
// PANEL SZACHOWNICY
// ==============================
class ChessPanel : public wxPanel {
public:
    Board             *board      = nullptr;
    int                selSq      = -1;
    std::vector<ChessMove> hints;
    ChessMove          lastMove;
    wxTextCtrl        *log        = nullptr;

    explicit ChessPanel(wxWindow *parent, wxWindowID id, Board *b)
        : wxPanel(parent, id, wxDefaultPosition, wxSize(PW, PH))
        , board(b)
        , lastMove(-1, -1, PieceType::NONE)
    {
        SetBackgroundColour(C_BG);
        Bind(wxEVT_PAINT,     &ChessPanel::onPaint, this);
        Bind(wxEVT_LEFT_DOWN, &ChessPanel::onClick, this);
    }

    void addLog(const wxString &msg) { if (log) log->AppendText(msg + "\n"); }

private:
    // ── Rysowanie ──────────────────────────────────────────────────────────
    void onPaint(wxPaintEvent &) {
        wxPaintDC dc(static_cast<wxWindow*>(this));
        draw(dc);
    }

    void draw(wxDC &dc) {
        // Etykiety osi
        dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dc.SetTextForeground(wxColour(140, 140, 160));
        for (int i = 0; i < 8; i++) {
            // kolumny (a..h)
            wxString col(static_cast<char>('a' + i));
            dc.DrawText(col, MAR + i * SQ + SQ/2 - 5, MAR + BSIZ + 6);
            // wiersze (1..8)
            wxString row = wxString::Format("%d", i + 1);
            dc.DrawText(row, 7, MAR + (7 - i) * SQ + SQ/2 - 8);
        }

        // Pola szachownicy
        for (int r = 0; r < 8; r++) {
            for (int f = 0; f < 8; f++) {
                int sq = r * 8 + f;
                int x  = MAR + f * SQ;
                int y  = MAR + (7 - r) * SQ;

                wxColour col = ((f + r) % 2 == 0) ? C_LIGHT : C_DARK;
                if (lastMove.from != -1 && (lastMove.from == sq || lastMove.to == sq))
                    col = C_HINT;
                else if (sq == selSq)
                    col = C_SEL;

                dc.SetBrush(wxBrush(col));
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.DrawRectangle(x, y, SQ, SQ);

                // Podpowiedzi legalnych ruchów
                if (selSq >= 0) {
                    for (const auto &m : hints) {
                        if (m.from == selSq && m.to == sq) {
                            dc.SetBrush(wxBrush(wxColour(100, 40, 210, 140)));
                            dc.SetPen(*wxTRANSPARENT_PEN);
                            dc.DrawCircle(x + SQ/2, y + SQ/2, 11);
                        }
                    }
                }
            }
        }

        // Figury (Unicode)
        const wchar_t wPieces[] = {L'♙', L'♘', L'♗', L'♖', L'♕', L'♔'};
        const wchar_t bPieces[] = {L'♟', L'♞', L'♝', L'♜', L'♛', L'♚'};

        dc.SetFont(wxFont(44, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

        for (int sq = 0; sq < 64; sq++) {
            int8_t piece = board->mailbox[sq];
            if (piece < 0) continue;

            int color = piece / 8;  // 0=biały,1=czarny
            int ptype = piece % 8;  // 0-5

            int x = MAR + (sq % 8) * SQ;
            int y = MAR + (7 - sq / 8) * SQ;

            wchar_t ch = (color == 0) ? wPieces[ptype] : bPieces[ptype];
            wxString glyph(ch);

            // Wyśrodkuj glif na polu
            wxCoord gw, gh;
            dc.GetTextExtent(glyph, &gw, &gh);
            int cx = x + (SQ - gw) / 2;
            int cy = y + (SQ - gh) / 2;

            // cień
            dc.SetTextForeground(wxColour(0, 0, 0, 90));
            dc.DrawText(glyph, cx + 2, cy + 2);
            // figura
            dc.SetTextForeground(color == 0 ? wxColour(255,255,255) : wxColour(20,20,20));
            dc.DrawText(glyph, cx, cy);
        }

        // Podświetlenie króla w szachu
        if (board->isInCheck(board->sideToMove)) {
            for (int sq = 0; sq < 64; sq++) {
                int8_t p = board->mailbox[sq];
                if (p < 0) continue;
                bool sideMatch = (board->sideToMove == Color::WHITE && p/8 == 0) ||
                                 (board->sideToMove == Color::BLACK && p/8 == 1);
                if (sideMatch && p % 8 == 5 /*KING*/) {
                    int x = MAR + (sq % 8) * SQ;
                    int y = MAR + (7 - sq / 8) * SQ;
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    dc.SetPen(wxPen(wxColour(220, 50, 50), 4));
                    dc.DrawRectangle(x + 2, y + 2, SQ - 4, SQ - 4);
                }
            }
        }
    }

    // ── Obsługa kliknięcia ─────────────────────────────────────────────────
    void onClick(wxMouseEvent &ev) {
        int mx = ev.GetX(), my = ev.GetY();
        if (mx < MAR || mx >= MAR + BSIZ || my < MAR || my >= MAR + BSIZ) return;

        int f  = (mx - MAR) / SQ;
        int r  = 7 - (my - MAR) / SQ;
        int sq = r * 8 + f;

        if (selSq != -1 && selSq != sq) {
            for (const auto &m : hints) {
                if (m.from == selSq && m.to == sq) {
                    board->makeMove(m, board->sideToMove);
                    lastMove = m;
                    selSq    = -1;
                    hints.clear();

                    wxWindow *self = static_cast<wxWindow*>(this);
                    self->Refresh();

                    GameStatus st = board->getGameStatus();
                    if (st != GameStatus::RUNNING) {
                        addLog(wxT("Koniec gry!"));
                        return;
                    }

                    addLog(wxT("Bot myśli..."));
                    wxYield();

                    auto res = engine::getBestMove(*board, 5);
                    if (res.bestMove.from != -1) {
                        board->makeMove(res.bestMove, board->sideToMove);
                        lastMove = res.bestMove;
                        wxString info;
                        info.Printf("Bot: %s | eval: %d | węzły: %lld",
                            mv2str(res.bestMove), res.score, (long long)res.nodes);
                        addLog(info);
                    }
                    self->Refresh();
                    return;
                }
            }
        }

        selSq = sq;
        hints = board->generateLegalMoves();
        wxWindow *self = static_cast<wxWindow*>(this);
        self->Refresh();
    }
};

// ==============================
// GŁÓWNA RAMKA
// ==============================
class ChessFrame : public wxFrame {
public:
    Board        board;
    ChessPanel  *panel;
    wxTextCtrl  *logBox;
    wxStaticText *turnLbl;

    ChessFrame()
        : wxFrame(nullptr, wxID_ANY, wxT("Szachy"),
                  wxDefaultPosition,
                  wxSize(PW + SW + 40, PH + 60),
                  wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX)
    {
        SetBackgroundColour(C_BG);

        wxPanel *root = new wxPanel(this, wxID_ANY);
        root->SetBackgroundColour(C_BG);

        panel = new ChessPanel(root, wxID_ANY, &board);

        // ── prawy panel ────────────────────────────────────────────────────
        wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);

        auto mkLabel = [&](const wxString &txt, int sz, bool bold,
                           wxColour fg = wxColour(220,220,240)) {
            auto *st = new wxStaticText(root, wxID_ANY, txt);
            wxFont f(sz, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                     bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
            st->SetFont(f);
            st->SetForegroundColour(fg);
            return st;
        };

        auto *title = mkLabel(wxT("♟  Szachy"), 22, true);
        turnLbl     = mkLabel(wxT("Ruch: Białe"), 13, true, wxColour(160,200,255));

        auto *logLbl = mkLabel(wxT("Historia ruchów:"), 10, false, wxColour(120,120,140));

        logBox = new wxTextCtrl(root, wxID_ANY,
            wxT("Gra rozpoczęta.\n"),
            wxDefaultPosition, wxSize(SW - 20, 300),
            wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_NONE);
        logBox->SetBackgroundColour(C_PANEL);
        logBox->SetForegroundColour(wxColour(200,200,220));

        auto *resetBtn = new wxButton(root, wxID_ANY, wxT("Nowa gra"));
        resetBtn->SetBackgroundColour(wxColour(100, 40, 210));
        resetBtn->SetForegroundColour(*wxWHITE);
        wxFont bf(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        resetBtn->SetFont(bf);
        resetBtn->Bind(wxEVT_BUTTON, &ChessFrame::onReset, this);

        right->AddSpacer(12);
        right->Add(title,    0, wxLEFT|wxRIGHT|wxBOTTOM, 12);
        right->Add(turnLbl,  0, wxLEFT|wxRIGHT|wxBOTTOM, 10);
        right->Add(logLbl,   0, wxLEFT|wxRIGHT, 10);
        right->Add(logBox,   1, wxALL|wxEXPAND, 10);
        right->Add(resetBtn, 0, wxALL|wxEXPAND, 10);
        right->AddSpacer(10);

        wxBoxSizer *main = new wxBoxSizer(wxHORIZONTAL);
        main->Add(static_cast<wxWindow*>(panel), 0, wxALL, 10);
        main->Add(right, 1, wxEXPAND|wxALL, 10);
        root->SetSizer(main);

        panel->log = logBox;

        // Odśwież etykietę tury po każdym przerysowaniu planszy
        panel->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) {
            e.Skip();
            refreshTurnLabel();
        });

        Centre();
    }

    void refreshTurnLabel() {
        if (!turnLbl) return;
        GameStatus st = board.getGameStatus();
        if (st == GameStatus::CHECKMATE) {
            wxString w = (board.sideToMove == Color::WHITE) ? "Czarne" : "Białe";
            turnLbl->SetLabel(wxString::Format("Mat! %s wygrały!", w));
            turnLbl->SetForegroundColour(wxColour(255, 80, 80));
        } else if (st != GameStatus::RUNNING) {
            turnLbl->SetLabel(wxT("Remis!"));
            turnLbl->SetForegroundColour(wxColour(200, 180, 80));
        } else {
            wxString s = (board.sideToMove == Color::WHITE) ? "Białe" : "Czarne";
            turnLbl->SetLabel(wxString::Format("Ruch: %s", s));
            turnLbl->SetForegroundColour(wxColour(160, 200, 255));
        }
    }

    void onReset(wxCommandEvent &) {
        board            = Board();
        panel->board     = &board;
        panel->selSq     = -1;
        panel->hints.clear();
        panel->lastMove  = ChessMove(-1, -1, PieceType::NONE);
        logBox->SetValue(wxT("Nowa gra. Kliknij figurę aby zacząć.\n"));
        refreshTurnLabel();
        wxWindow *w = static_cast<wxWindow*>(panel);
        w->Refresh();
    }
};

// ==============================
// APLIKACJA
// ==============================
class ChessApp : public wxApp {
public:
    bool OnInit() override {
        init_magic_tables();
        init_zobrist();

        // Próba załadowania wag NNUE z kilku lokalizacji
        if (!engine::loadNNUE("nn.bin")) {
            engine::loadNNUE("../nn.bin");
        }

        (new ChessFrame())->Show();
        return true;
    }
};

wxIMPLEMENT_APP(ChessApp);
