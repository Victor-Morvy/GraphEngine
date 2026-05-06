#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include <iostream>

#include "graphengine.h"

static QTextStream out(stdout);

// ─────────────────────────────────────────────────────────────────────────────

static void printResult(const QString &from, const QString &to,
                        const GraphEngine::PathResult &r)
{
    const QString header = QString("  %1 → %2").arg(from, -10).arg(to, -10);
    out << header << " : ";
    if (r.found)
        out << r.toString();
    else
        out << "SEM CAMINHO";
    out << "\n";
    out.flush();
}

static void section(const QString &title)
{
    out << "\n────────────────────────────────────────────────────────\n";
    out << "  " << title << "\n";
    out << "────────────────────────────────────────────────────────\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Fallback graph usado quando grafo.json não existe.
// Representa um mapa de cômodos:
//
//   Entrada ─1─► Sala ─2─► Quarto ─1─► Garagem
//      │                               ▲
//      └─1─► Cozinha ─2─► Banheiro ─1─┘
//
// ─────────────────────────────────────────────────────────────────────────────
static void buildFallbackGraph(GraphEngine &g)
{
    g.addNode("Entrada");  g.addNode("Sala");
    g.addNode("Cozinha");  g.addNode("Quarto");
    g.addNode("Banheiro"); g.addNode("Garagem");

    g.addEdge("Entrada",  "Sala",      1.0);
    g.addEdge("Entrada",  "Cozinha",   1.0);
    g.addEdge("Sala",     "Quarto",    2.0);
    g.addEdge("Cozinha",  "Banheiro",  2.0);
    g.addEdge("Quarto",   "Garagem",   1.0);
    g.addEdge("Banheiro", "Garagem",   1.0);
    g.addEdge("Sala",     "Banheiro",  3.0); // cross-link
    g.addEdge("Cozinha",  "Quarto",    3.0); // cross-link
}

// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // out << "╔═══════════════════════════════════════════════════════╗\n";
    // out << "║             GraphEngine  —  C++ Demo                 ║\n";
    // out << "╚═══════════════════════════════════════════════════════╝\n";

    // ── Carregamento ──────────────────────────────────────────────────────────
    GraphEngine g;
    const QString jsonPath = (argc > 1) ? QString::fromLocal8Bit(argv[1])
                                        // : QStringLiteral("grafo.json");
                                        : QStringLiteral("C:/Users/Victor/Documents/Repositories/GraphEngine/grafo.json");
    bool fromFile = g.loadFromFile(jsonPath);

    if (fromFile) {
        out << "\nGrafo carregado de: " << jsonPath << "\n";
    } else {
        out << "\n[AVISO] '" << jsonPath << "' não encontrado — usando grafo embutido.\n";
        buildFallbackGraph(g);
    }

    out << "  Nós    : " << g.nodeCount()               << "\n";
    out << "  Arestas: " << g.edgeCount()               << "\n";
    out << "  Nomes  : " << g.nodeNames().join(", ")    << "\n";

    // Coleta os nós disponíveis para testes dinâmicos
    const QStringList nodes = g.nodeNames();
    const int n = nodes.size();

    std::cout << "g.findPath(\"F\", \"I\") " << g.findPath("F", "I").toString().toStdString() << std::endl;

    // // ── Teste 1: todos os nós livres — pares variados ─────────────────────────
    // section("Teste 1 — Pathfinding com todos os nós livres");

    // if (n >= 2) {
    //     // Testa vários pares distribuídos pela lista
    //     const QList<QPair<int,int>> pairs = {
    //         {0, n-1}, {0, n/2}, {n/3, n-1}, {n/4, 3*n/4}
    //     };
    //     for (const auto &[a, b] : pairs) {
    //         if (a != b)
    //             printResult(nodes[a], nodes[b], g.findPath(nodes[a], nodes[b]));
    //     }
    // }

    // // ── Teste 2: bloqueio de nó intermediário ─────────────────────────────────
    // section("Teste 2 — Bloqueando nó intermediário");

    // if (n >= 3) {
    //     const QString blocked = nodes[n / 2]; // nó do meio como obstáculo
    //     const QString src     = nodes[0];
    //     const QString dst     = nodes[n - 1];

    //     out << "  [livre   ] ";
    //     printResult(src, dst, g.findPath(src, dst));

    //     g.blockNode(blocked);
    //     out << "  [bloq: " << blocked << "] ";
    //     printResult(src, dst, g.findPath(src, dst));

    //     // destino bloqueado
    //     out << "  [dest bloqueado] ";
    //     printResult(src, blocked, g.findPath(src, blocked));

    //     g.clearBlocked();
    //     out << "  [desbloqueado  ] ";
    //     printResult(src, dst, g.findPath(src, dst));
    // }

    // // ── Teste 3: bloqueio em cadeia ───────────────────────────────────────────
    // section("Teste 3 — Bloqueando múltiplos nós");

    // if (n >= 4) {
    //     const QString src = nodes[0];
    //     const QString dst = nodes[n - 1];

    //     // Bloqueia o primeiro terço dos nós (exceto src e dst)
    //     QStringList blocked;
    //     for (int i = 1; i <= n / 3; ++i) {
    //         if (nodes[i] != src && nodes[i] != dst) {
    //             g.blockNode(nodes[i]);
    //             blocked << nodes[i];
    //         }
    //     }

    //     out << "  [bloqueados: " << blocked.join(", ") << "]\n\n";
    //     printResult(src, dst, g.findPath(src, dst));
    //     g.clearBlocked();
    // }

    // // ── Teste 4: API manual — addNode + addEdge em runtime ────────────────────
    // section("Teste 4 — addNode / addEdge em runtime");

    // g.addNode("Deposito");
    // g.addNode("Saida");

    // // Conecta "Deposito" ao primeiro nó, e "Saida" ao último
    // g.addEdge(nodes[n - 1], "Deposito",  5.0);
    // g.addEdge("Deposito",   "Saida",     2.0);
    // g.addEdge(nodes[0],     "Saida",    20.0); // rota longa direta

    // out << "  Novos nós: Deposito, Saida  (total: " << g.nodeCount() << ")\n\n";

    // printResult(nodes[0],   "Saida",    g.findPath(nodes[0],   "Saida"));
    // printResult(nodes[0],   "Deposito", g.findPath(nodes[0],   "Deposito"));
    // printResult("Deposito", "Saida",    g.findPath("Deposito", "Saida"));

    // // ── Teste 5: casos extremos ───────────────────────────────────────────────
    // section("Teste 5 — Casos extremos");

    // // Mesmo nó
    // printResult(nodes[0], nodes[0], g.findPath(nodes[0], nodes[0]));

    // // Nó isolado (sem arestas)
    // g.addNode("Isolado");
    // printResult(nodes[0], "Isolado", g.findPath(nodes[0], "Isolado"));
    // printResult("Isolado", nodes[0], g.findPath("Isolado", nodes[0]));

    // // Nó inexistente
    // printResult(nodes[0], "???", g.findPath(nodes[0], "???"));

    // // ── Resumo ────────────────────────────────────────────────────────────────
    // out << "\n╔═══════════════════════════════════════════════════════╗\n";
    // out << "║                  Fim dos testes                      ║\n";
    // out << "╚═══════════════════════════════════════════════════════╝\n";

    return 0;
}
