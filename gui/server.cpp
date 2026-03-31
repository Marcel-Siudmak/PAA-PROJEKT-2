// gui/server.cpp - Prosty HTTP serwer szachowy dla GUI w przeglądarce
// Kompilacja: dodaj do CMakeLists.txt jako chess_gui

#include "board.hpp"
#include "magic.hpp"
#include "move.hpp"
#include "search.hpp"
#include "types.hpp"
#include "zobrist.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <fstream>

// ==============================
// STAN GLOBALNY GRY
// ==============================
static Board g_board;
static std::string g_lastBotInfo = "";

// ==============================
// POMOCNICZE FUNKCJE
// ==============================

std::string sqToNotation(int sq) {
    return std::string(1, static_cast<char>('a' + (sq % 8))) +
           std::to_string((sq / 8) + 1);
}

std::string moveToString(const Move &m) {
    std::string promo = "";
    if (m.promotion != PieceType::NONE) {
        if (m.promotion == PieceType::QUEEN)  promo = "q";
        else if (m.promotion == PieceType::ROOK)   promo = "r";
        else if (m.promotion == PieceType::BISHOP) promo = "b";
        else if (m.promotion == PieceType::KNIGHT) promo = "n";
    }
    return sqToNotation(m.from) + sqToNotation(m.to) + promo;
}

// Zamień specjalne znaki na bezpieczne JSON
std::string jsonEscape(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

// ==============================
// GENEROWANIE JSON STANU GRY
// ==============================

std::string getBoardJson() {
    std::ostringstream json;
    json << "{";

    // Figury: tablica 64 elementów, każdy to string np. "wP", "bK" lub null
    json << "\"pieces\":[";
    const char* pieceLetters = "PNBRQKpnbrqk"; // biała uppercase, czarna lowercase
    for (int sq = 0; sq < 64; sq++) {
        if (sq > 0) json << ",";
        int8_t mb = g_board.mailbox[sq];
        if (mb < 0) {
            json << "null";
        } else {
            int color = mb / 8;      // 0=biały, 1=czarny
            int ptype = mb % 8;      // 0-5 = typ figury
            // Kolor: "w" lub "b", typ: P N B R Q K
            char c = (color == 0) ? 'w' : 'b';
            char p = "PNBRQK"[ptype];
            json << "\"" << c << p << "\"";
        }
    }
    json << "],";

    // Czyja kolej
    json << "\"sideToMove\":\"" << (g_board.sideToMove == Color::WHITE ? "white" : "black") << "\",";

    // Status gry
    GameStatus status = g_board.getGameStatus();
    std::string statusStr = "running";
    std::string statusMessage = "";
    if (status == GameStatus::CHECKMATE) {
        statusStr = "checkmate";
        statusMessage = (g_board.sideToMove == Color::WHITE) ? "Czarne wygrały!" : "Białe wygrały!";
    } else if (status == GameStatus::STALEMATE) {
        statusStr = "draw";
        statusMessage = "Remis - Pat!";
    } else if (status == GameStatus::DRAW_FIFTY_MOVES) {
        statusStr = "draw";
        statusMessage = "Remis - Zasada 50 ruchów!";
    } else if (status == GameStatus::DRAW_REPETITION) {
        statusStr = "draw";
        statusMessage = "Remis - Trzykrotne powtórzenie!";
    } else if (status == GameStatus::DRAW_INSUFFICIENT_MATERIAL) {
        statusStr = "draw";
        statusMessage = "Remis - Niedostateczny materiał!";
    }
    json << "\"status\":\"" << statusStr << "\",";
    json << "\"statusMessage\":\"" << jsonEscape(statusMessage) << "\",";

    // Szach?
    bool inCheck = (status == GameStatus::RUNNING) && g_board.isInCheck(g_board.sideToMove);
    json << "\"inCheck\":" << (inCheck ? "true" : "false") << ",";

    // Legalne ruchy (lista stringów np. "e2e4")
    json << "\"legalMoves\":[";
    if (status == GameStatus::RUNNING) {
        std::vector<Move> moves = g_board.generateLegalMoves();
        bool first = true;
        for (const auto &m : moves) {
            if (!first) json << ",";
            json << "\"" << moveToString(m) << "\"";
            first = false;
        }
    }
    json << "],";

    // Info o ostatnim ruchu bota
    json << "\"botInfo\":\"" << jsonEscape(g_lastBotInfo) << "\"";

    json << "}";
    return json.str();
}

// ==============================
// HTTP PARSING & RESPONSE
// ==============================

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

HttpRequest parseRequest(const std::string &raw) {
    HttpRequest req;
    std::istringstream ss(raw);
    ss >> req.method >> req.path;

    // Znajdź body (po \r\n\r\n)
    size_t bodyStart = raw.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        req.body = raw.substr(bodyStart + 4);
    }
    return req;
}

std::string httpResponse(int code, const std::string &contentType, const std::string &body) {
    std::string status = (code == 200) ? "200 OK" : (code == 400 ? "400 Bad Request" : "404 Not Found");
    std::ostringstream resp;
    resp << "HTTP/1.1 " << status << "\r\n";
    resp << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    resp << "Access-Control-Allow-Origin: *\r\n";
    resp << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    resp << "Access-Control-Allow-Headers: Content-Type\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    return resp.str();
}

// Prosta ekstrakcja wartości ze stringa JSON: {"key":"value"} -> value
std::string extractJsonString(const std::string &json, const std::string &key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    start += search.size();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

// Wczytaj plik HTML
std::string readFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// ==============================
// OBSŁUGA REQUESTÓW
// ==============================

std::string handleRequest(const HttpRequest &req) {
    // CORS preflight
    if (req.method == "OPTIONS") {
        return httpResponse(200, "text/plain", "");
    }

    // GET / -> serwuj index.html
    if (req.method == "GET" && (req.path == "/" || req.path == "/index.html")) {
        // Szukaj index.html przy pliku wykonywalnym
        std::string htmlPath = "gui/index.html";
        std::string html = readFile(htmlPath);
        if (html.empty()) {
            // spróbuj z bieżącego katalogu
            htmlPath = "index.html";
            html = readFile(htmlPath);
        }
        if (html.empty()) {
            return httpResponse(404, "text/plain", "index.html not found");
        }
        return httpResponse(200, "text/html", html);
    }

    // GET /state -> zwróć stan planszy jako JSON
    if (req.method == "GET" && req.path == "/state") {
        return httpResponse(200, "application/json", getBoardJson());
    }

    // POST /move {"move":"e2e4"}
    if (req.method == "POST" && req.path == "/move") {
        std::string moveStr = extractJsonString(req.body, "move");
        if (moveStr.empty()) {
            return httpResponse(400, "application/json", "{\"error\":\"Brak pola 'move'\"}");
        }

        GameStatus st = g_board.getGameStatus();
        if (st != GameStatus::RUNNING) {
            return httpResponse(400, "application/json", "{\"error\":\"Gra zakonczona\"}");
        }

        std::vector<Move> moves = g_board.generateLegalMoves();
        bool found = false;
        for (const auto &m : moves) {
            if (moveToString(m) == moveStr) {
                g_board.makeMove(m, g_board.sideToMove);
                found = true;
                break;
            }
        }

        if (!found) {
            return httpResponse(400, "application/json", "{\"error\":\"Nieprawidlowy ruch\"}");
        }

        g_lastBotInfo = "";
        return httpResponse(200, "application/json", getBoardJson());
    }

    // POST /bot -> bota ruch
    if (req.method == "POST" && req.path == "/bot") {
        GameStatus st = g_board.getGameStatus();
        if (st != GameStatus::RUNNING) {
            return httpResponse(400, "application/json", "{\"error\":\"Gra zakonczona\"}");
        }

        engine::SearchResult res = engine::getBestMove(g_board, 7);
        std::string mv = moveToString(res.bestMove);
        g_board.makeMove(res.bestMove, g_board.sideToMove);

        std::ostringstream info;
        info << "Bot zagrał: " << mv
             << " | eval: " << res.score
             << " | węzły: " << res.nodes
             << " | czas: " << res.timeSeconds << "s";
        g_lastBotInfo = info.str();

        return httpResponse(200, "application/json", getBoardJson());
    }

    // POST /reset -> resetuj grę
    if (req.method == "POST" && req.path == "/reset") {
        g_board = Board();
        g_lastBotInfo = "";
        return httpResponse(200, "application/json", getBoardJson());
    }

    return httpResponse(404, "application/json", "{\"error\":\"Not found\"}");
}

// ==============================
// OBSŁUGA POŁĄCZENIA W WĄTKU
// ==============================

void handleClient(int clientFd) {
    std::string raw;
    char buf[4096];

    // 1. Czytaj aż do \r\n\r\n (koniec nagłówków)
    while (raw.find("\r\n\r\n") == std::string::npos) {
        int n = recv(clientFd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        raw.append(buf, n);
    }

    // 2. Jeśli jest Content-Length, doczytaj body
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headers = raw.substr(0, headerEnd);
        std::string body    = raw.substr(headerEnd + 4);

        // Szukaj Content-Length (case-insensitive workaround)
        int contentLength = 0;
        for (const auto& prefix : {"Content-Length: ", "content-length: "}) {
            size_t pos = headers.find(prefix);
            if (pos != std::string::npos) {
                size_t end = headers.find("\r\n", pos);
                contentLength = std::stoi(headers.substr(pos + strlen(prefix),
                                                          end - pos - strlen(prefix)));
                break;
            }
        }

        // Doczytaj brakujące bajty body
        while ((int)body.size() < contentLength) {
            int n = recv(clientFd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            body.append(buf, n);
        }

        // Złóż z powrotem
        raw = raw.substr(0, headerEnd + 4) + body;
    }

    HttpRequest req = parseRequest(raw);
    std::string response = handleRequest(req);
    send(clientFd, response.c_str(), response.size(), 0);
    close(clientFd);
}

// ==============================
// MAIN
// ==============================

int main() {
    srand(time(NULL));
    init_magic_tables();
    init_zobrist();

    int port = 8080;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Błąd tworzenia socketu\n";
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Błąd bind na porcie " << port << "\n";
        return 1;
    }

    listen(serverFd, 10);

    std::cout << "=================================\n";
    std::cout << "  Serwer szachowy uruchomiony!\n";
    std::cout << "  Otwórz: http://localhost:" << port << "\n";
    std::cout << "=================================\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr *)&clientAddr, &clientLen);
        if (clientFd < 0) continue;

        // Obsługuj każde połączenie w osobnym wątku
        std::thread t(handleClient, clientFd);
        t.detach();
    }

    close(serverFd);
    return 0;
}
