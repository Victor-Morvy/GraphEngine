# GraphEngine

Editor visual de grafos direcionados com pesos, exportação JSON e biblioteca C++ com Dijkstra.

O projeto tem dois componentes independentes que se comunicam via JSON:

| Componente | Tecnologia | Função |
|---|---|---|
| `index.html` | HTML5 / Canvas / JS | Editor visual — cria, edita, analisa e exporta grafos |
| `lib/` | C++ / Qt | Biblioteca estática — carrega o JSON e faz pathfinding |

---

## Editor HTML (`index.html`)

Abra diretamente no navegador — sem servidor, sem dependências.

### Modos de edição

| Atalho | Modo | Ação |
|---|---|---|
| `S` | Selecionar | Clica/arrasta nós; clica arestas para selecionar |
| `N` | Adicionar Nó | Clique no canvas cria um nó com label automático |
| `E` | Adicionar Aresta | Clique em dois nós; popup define direção e custo |
| `D` / `Del` | Deletar | Clique no elemento para remover |
| `Esc` | — | Cancela ação atual |

- **Duplo-clique** em nó → renomear  
- **Duplo-clique** em aresta → editar custo  
- **Botão direito** → menu de contexto  
- **Arrastar fundo** ou **scroll** → navegar no canvas  

### Arestas

Ao conectar dois nós aparece um popup com três opções:

```
→ A → B          (aresta simples, A para B)
← B → A          (aresta simples, B para A)
↔ Bidirecional   (cria as duas arestas)
```

Cada aresta tem um **custo** (padrão `1`).

### Modo Play ▶

Ativa o modo de teste em runtime:

- Clique nos nós para **bloqueá-los / desbloqueá-los** (válvulas fechadas)
- Nós bloqueados são ignorados no pathfinding e na análise de fluxo
- Painel lateral mostra resultado: caminho, custo total e número de arestas

Algoritmos disponíveis: **Dijkstra** (custo mínimo) e **BFS** (menos saltos).

### Análise de Fluxo ⬡

Simula líquido escoando a partir de um **nó emissor** pelo grafo direcionado:

| Visual | Significado |
|---|---|
| Âmbar pulsante + badge `EMIT` | Nó emissor (fonte do fluxo) |
| Ciano + glow | Nó alcançável pelo fluxo |
| Dashes ciano animados | Aresta carregando fluxo (animação no sentido da seta) |
| Vermelho + badge `BLOQ` | Válvula fechada — fluxo para antes deste nó |
| 20% opacidade | Elemento inalcançável |

**Regras do fluxo:**
- Segue apenas o sentido das setas (grafo direcionado)
- Para em nós bloqueados (válvulas fechadas)
- Uma aresta só é marcada como fluindo se `dist(from) < dist(to)` — o fluxo nunca volta por onde já passou, mesmo que exista aresta reversa
- Re-analisa automaticamente ao bloquear/desbloquear nós no modo Play

### Persistência

- **Auto-save** em `localStorage` a cada ação — carrega automaticamente ao reabrir
- **Exportar** → baixa `grafo.json`
- **Carregar** → importa um `grafo.json` existente

---

## Biblioteca C++ (`lib/`)

Requer **Qt 6** e **Qt Creator** com MSVC 2019+ ou MinGW.

### Compilar

```
# Abra GraphEngine.pro no Qt Creator e clique em Build.
# A lib compila primeiro (dependência declarada no subdirs .pro).
```

Ou via linha de comando:

```bash
cd build
qmake ../GraphEngine.pro
make          # ou nmake / jom no Windows
```

O `grafo.json` é copiado automaticamente ao lado do executável após cada build.

### API

```cpp
#include "graphengine.h"

GraphEngine g;

// ── Carregar do JSON exportado pelo editor ──────────────────────────────────
g.loadFromFile("grafo.json");

// ── Ou construir manualmente ────────────────────────────────────────────────
g.addNode("A");
g.addNode("B");
g.addEdge("A", "B", 2.5);   // A → B, custo 2.5
g.addEdge("B", "A", 1.0);   // bidirecional: B → A, custo 1.0

// ── Bloquear nós em runtime ─────────────────────────────────────────────────
g.blockNode("B");            // B será ignorado no pathfinding
g.isBlocked("B");            // true
g.clearBlocked();            // desbloqueia todos

// ── Pathfinding — Dijkstra ──────────────────────────────────────────────────
GraphEngine::PathResult r = g.findPath("A", "C");

r.found      // bool
r.path       // QStringList{"A", "B", "C"}
r.totalCost  // double — soma dos custos das arestas
r.steps      // int    — número de arestas percorridas
r.toString() // QString formatada para exibição
```

### Estrutura interna

Internamente todos os nós são indexados por `int` (`NodeId`). `QString` só toca o grafo na fronteira pública — uma única lookup de hash por chamada. O loop do Dijkstra opera exclusivamente sobre inteiros e `double[]` contíguos.

```
label → NodeId   (QHash, 1× por chamada pública)
m_nodes[id]      (QVector, acesso O(1) por índice)
m_adj[fromId]    (lista de adjacência, Dijkstra itera só vizinhos)
dist[] / prev[]  (std::vector<double/int>, cache-friendly)
```

---

## Estrutura do repositório

```
GraphEngine/
├── index.html          # editor visual (abrir no browser)
├── grafo.json          # grafo de exemplo exportado pelo editor
├── GraphEngine.pro     # projeto Qt Creator (subdirs)
├── lib/
│   ├── lib.pro
│   ├── graphengine.h
│   └── graphengine.cpp
└── app/
    ├── app.pro
    └── main.cpp        # demo: carrega grafo.json e testa pathfinding
```

## Formato JSON

O JSON exportado pelo editor é o mesmo consumido pela biblioteca:

```json
{
  "version": 1,
  "nodes": [
    { "id": "n1", "x": 100, "y": 200, "label": "Entrada" }
  ],
  "edges": [
    { "id": "e1", "from": "n1", "to": "n2", "cost": 1 }
  ]
}
```
