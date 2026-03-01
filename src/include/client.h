#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

extern int rows;         // The count of rows of the game map.
extern int columns;      // The count of columns of the game map.
extern int total_mines;  // The count of mines of the game map.

// You MUST NOT use any other external variables except for rows, columns and total_mines.

/**
 * @brief The definition of function Execute(int, int, bool)
 *
 * @details This function is designed to take a step when player the client's (or player's) role, and the implementation
 * of it has been finished by TA. (I hope my comments in code would be easy to understand T_T) If you do not understand
 * the contents, please ask TA for help immediately!!!
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 * @param type The type of operation to a certain block.
 * If type == 0, we'll execute VisitBlock(row, column).
 * If type == 1, we'll execute MarkMine(row, column).
 * If type == 2, we'll execute AutoExplore(row, column).
 * You should not call this function with other type values.
 */
void Execute(int r, int c, int type);

namespace client_internal {
constexpr int kMaxN = 35;
constexpr int kUnknown = -2;
constexpr int kFlagged = -3;

struct Action {
  int r, c, type;
};

struct Constraint {
  std::vector<int> vars;
  int need;
};

int board[kMaxN][kMaxN];
std::deque<Action> action_queue;
bool queued_visit[kMaxN][kMaxN];
bool queued_mark[kMaxN][kMaxN];
bool queued_auto[kMaxN][kMaxN];

double exact_prob[kMaxN][kMaxN];
bool has_exact_prob[kMaxN][kMaxN];

const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

inline bool InBounds(int r, int c) {
  return 0 <= r && r < rows && 0 <= c && c < columns;
}

inline int CellId(int r, int c) {
  return r * columns + c;
}

inline std::pair<int, int> DecodeId(int id) {
  return {id / columns, id % columns};
}

void ResetQueues() {
  action_queue.clear();
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      queued_visit[i][j] = false;
      queued_mark[i][j] = false;
      queued_auto[i][j] = false;
    }
  }
}

void ClearExactProb() {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      has_exact_prob[i][j] = false;
      exact_prob[i][j] = 0.0;
    }
  }
}

void EnqueueVisit(int r, int c) {
  if (!InBounds(r, c) || board[r][c] != kUnknown || queued_visit[r][c]) {
    return;
  }
  queued_visit[r][c] = true;
  action_queue.push_back({r, c, 0});
}

void EnqueueMark(int r, int c) {
  if (!InBounds(r, c) || board[r][c] != kUnknown || queued_mark[r][c]) {
    return;
  }
  queued_mark[r][c] = true;
  action_queue.push_back({r, c, 1});
}

void EnqueueAuto(int r, int c) {
  if (!InBounds(r, c) || board[r][c] < 0 || queued_auto[r][c]) {
    return;
  }
  queued_auto[r][c] = true;
  action_queue.push_back({r, c, 2});
}

bool ValidAction(const Action &a) {
  if (!InBounds(a.r, a.c)) {
    return false;
  }
  if (a.type == 0 || a.type == 1) {
    return board[a.r][a.c] == kUnknown;
  }
  if (a.type == 2) {
    if (board[a.r][a.c] < 0) {
      return false;
    }
    int clue = board[a.r][a.c];
    int flagged = 0, unknown = 0;
    for (int k = 0; k < 8; ++k) {
      int nr = a.r + dr[k], nc = a.c + dc[k];
      if (!InBounds(nr, nc)) {
        continue;
      }
      if (board[nr][nc] == kFlagged) {
        ++flagged;
      } else if (board[nr][nc] == kUnknown) {
        ++unknown;
      }
    }
    return unknown > 0 && flagged == clue;
  }
  return false;
}

bool ExecuteFromQueue() {
  while (!action_queue.empty()) {
    Action a = action_queue.front();
    action_queue.pop_front();
    if (a.type == 0) {
      queued_visit[a.r][a.c] = false;
    } else if (a.type == 1) {
      queued_mark[a.r][a.c] = false;
    } else {
      queued_auto[a.r][a.c] = false;
    }

    if (!ValidAction(a)) {
      continue;
    }
    Execute(a.r, a.c, a.type);
    return true;
  }
  return false;
}

bool ApplyBasicInference() {
  bool changed = false;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] < 0) {
        continue;
      }
      int clue = board[r][c];
      int flagged = 0;
      std::vector<std::pair<int, int>> unknowns;
      for (int k = 0; k < 8; ++k) {
        int nr = r + dr[k], nc = c + dc[k];
        if (!InBounds(nr, nc)) {
          continue;
        }
        if (board[nr][nc] == kFlagged) {
          ++flagged;
        } else if (board[nr][nc] == kUnknown) {
          unknowns.push_back({nr, nc});
        }
      }
      if (unknowns.empty()) {
        continue;
      }

      int need = clue - flagged;
      if (need == 0) {
        EnqueueAuto(r, c);
        for (auto &u : unknowns) {
          EnqueueVisit(u.first, u.second);
        }
        changed = true;
      } else if (need == static_cast<int>(unknowns.size())) {
        for (auto &u : unknowns) {
          EnqueueMark(u.first, u.second);
        }
        changed = true;
      }
    }
  }
  return changed;
}

bool IsSubset(const std::vector<int> &a, const std::vector<int> &b) {
  if (a.size() > b.size()) {
    return false;
  }
  int i = 0, j = 0;
  while (i < static_cast<int>(a.size()) && j < static_cast<int>(b.size())) {
    if (a[i] == b[j]) {
      ++i;
      ++j;
    } else if (a[i] > b[j]) {
      ++j;
    } else {
      return false;
    }
  }
  return i == static_cast<int>(a.size());
}

std::vector<int> SetDiff(const std::vector<int> &a, const std::vector<int> &b) {
  // return b - a, assuming a is subset of b
  std::vector<int> ret;
  int i = 0, j = 0;
  while (j < static_cast<int>(b.size())) {
    if (i < static_cast<int>(a.size()) && a[i] == b[j]) {
      ++i;
      ++j;
    } else {
      ret.push_back(b[j]);
      ++j;
    }
  }
  return ret;
}

std::vector<Constraint> BuildConstraints() {
  std::vector<Constraint> constraints;
  constraints.reserve(rows * columns);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] < 0) {
        continue;
      }
      int clue = board[r][c];
      int flagged = 0;
      std::vector<int> unknown_ids;
      for (int k = 0; k < 8; ++k) {
        int nr = r + dr[k], nc = c + dc[k];
        if (!InBounds(nr, nc)) {
          continue;
        }
        if (board[nr][nc] == kFlagged) {
          ++flagged;
        } else if (board[nr][nc] == kUnknown) {
          unknown_ids.push_back(CellId(nr, nc));
        }
      }
      if (unknown_ids.empty()) {
        continue;
      }
      std::sort(unknown_ids.begin(), unknown_ids.end());
      unknown_ids.erase(std::unique(unknown_ids.begin(), unknown_ids.end()), unknown_ids.end());
      int need = clue - flagged;
      if (0 <= need && need <= static_cast<int>(unknown_ids.size())) {
        constraints.push_back({unknown_ids, need});
      }
    }
  }
  return constraints;
}

bool ApplySubsetInference(const std::vector<Constraint> &constraints) {
  bool changed = false;
  int n = static_cast<int>(constraints.size());
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (i == j) {
        continue;
      }
      const auto &A = constraints[i].vars;
      const auto &B = constraints[j].vars;
      if (A.empty() || B.empty() || A == B) {
        continue;
      }
      if (!IsSubset(A, B)) {
        continue;
      }
      int need_diff = constraints[j].need - constraints[i].need;
      auto diff = SetDiff(A, B);
      if (diff.empty()) {
        continue;
      }
      if (need_diff == 0) {
        for (int id : diff) {
          auto [r, c] = DecodeId(id);
          EnqueueVisit(r, c);
        }
        changed = true;
      } else if (need_diff == static_cast<int>(diff.size())) {
        for (int id : diff) {
          auto [r, c] = DecodeId(id);
          EnqueueMark(r, c);
        }
        changed = true;
      }
    }
  }
  return changed;
}

bool ApplyExactComponentInference(const std::vector<Constraint> &constraints) {
  // Build frontier variable set
  std::vector<int> frontier_ids;
  frontier_ids.reserve(rows * columns);
  {
    std::vector<int> seen(rows * columns, 0);
    for (const auto &con : constraints) {
      for (int id : con.vars) {
        if (!seen[id]) {
          seen[id] = 1;
          frontier_ids.push_back(id);
        }
      }
    }
  }

  if (frontier_ids.empty()) {
    return false;
  }

  std::unordered_map<int, int> id_to_idx;
  id_to_idx.reserve(frontier_ids.size() * 2 + 1);
  for (int i = 0; i < static_cast<int>(frontier_ids.size()); ++i) {
    id_to_idx[frontier_ids[i]] = i;
  }

  // Graph on frontier variables: connect variables that appear in same constraint.
  int V = static_cast<int>(frontier_ids.size());
  std::vector<std::vector<int>> var_graph(V);
  std::vector<std::vector<int>> constraint_vars_idx;
  constraint_vars_idx.reserve(constraints.size());

  for (const auto &con : constraints) {
    std::vector<int> ids;
    ids.reserve(con.vars.size());
    for (int cid : con.vars) {
      ids.push_back(id_to_idx[cid]);
    }
    constraint_vars_idx.push_back(ids);
    for (int i = 1; i < static_cast<int>(ids.size()); ++i) {
      int a = ids[i - 1], b = ids[i];
      var_graph[a].push_back(b);
      var_graph[b].push_back(a);
    }
  }

  bool changed = false;
  std::vector<int> comp_of_var(V, -1);
  int comp_cnt = 0;
  for (int i = 0; i < V; ++i) {
    if (comp_of_var[i] != -1) {
      continue;
    }
    std::queue<int> q;
    q.push(i);
    comp_of_var[i] = comp_cnt;
    while (!q.empty()) {
      int u = q.front();
      q.pop();
      for (int v : var_graph[u]) {
        if (comp_of_var[v] == -1) {
          comp_of_var[v] = comp_cnt;
          q.push(v);
        }
      }
    }
    ++comp_cnt;
  }

  std::vector<std::vector<int>> comp_vars(comp_cnt);
  for (int i = 0; i < V; ++i) {
    comp_vars[comp_of_var[i]].push_back(i);
  }

  // Assign constraints to components using first variable.
  std::vector<std::vector<int>> comp_constraints(comp_cnt);
  for (int ci = 0; ci < static_cast<int>(constraints.size()); ++ci) {
    if (constraint_vars_idx[ci].empty()) {
      continue;
    }
    int comp = comp_of_var[constraint_vars_idx[ci][0]];
    comp_constraints[comp].push_back(ci);
  }

  constexpr int kExactLimit = 26;

  for (int comp = 0; comp < comp_cnt; ++comp) {
    auto &vars = comp_vars[comp];
    if (vars.empty()) {
      continue;
    }

    // If component is too large, skip expensive exact enumeration.
    if (static_cast<int>(vars.size()) > kExactLimit) {
      continue;
    }

    std::unordered_map<int, int> local_idx;
    local_idx.reserve(vars.size() * 2 + 1);
    for (int i = 0; i < static_cast<int>(vars.size()); ++i) {
      local_idx[vars[i]] = i;
    }

    struct LocalConstraint {
      std::vector<int> lv;
      int need;
    };
    std::vector<LocalConstraint> local_cons;
    local_cons.reserve(comp_constraints[comp].size());

    for (int ci : comp_constraints[comp]) {
      LocalConstraint lc;
      lc.need = constraints[ci].need;
      for (int gv : constraint_vars_idx[ci]) {
        lc.lv.push_back(local_idx[gv]);
      }
      local_cons.push_back(std::move(lc));
    }

    int n = static_cast<int>(vars.size());
    int m = static_cast<int>(local_cons.size());
    std::vector<std::vector<int>> var_to_cons(n);
    for (int ci = 0; ci < m; ++ci) {
      for (int v : local_cons[ci].lv) {
        var_to_cons[v].push_back(ci);
      }
    }

    // Variable order by degree (high first) for stronger pruning.
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) {
      order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
      return var_to_cons[a].size() > var_to_cons[b].size();
    });

    std::vector<int> assign(n, -1);
    std::vector<int> assigned_cnt(m, 0), mine_cnt(m, 0);

    long double total_ways = 0.0L;
    std::vector<long double> mine_ways(n, 0.0L);

    auto feasible = [&](int ci) {
      int assigned = assigned_cnt[ci];
      int mines = mine_cnt[ci];
      int size = static_cast<int>(local_cons[ci].lv.size());
      int need = local_cons[ci].need;
      if (mines > need) {
        return false;
      }
      int max_possible = mines + (size - assigned);
      if (max_possible < need) {
        return false;
      }
      return true;
    };

    std::function<void(int)> dfs = [&](int idx) {
      if (idx == n) {
        for (int ci = 0; ci < m; ++ci) {
          if (mine_cnt[ci] != local_cons[ci].need) {
            return;
          }
        }
        total_ways += 1.0L;
        for (int i = 0; i < n; ++i) {
          if (assign[i] == 1) {
            mine_ways[i] += 1.0L;
          }
        }
        return;
      }

      int v = order[idx];
      for (int val = 0; val <= 1; ++val) {
        assign[v] = val;
        bool ok = true;
        for (int ci : var_to_cons[v]) {
          ++assigned_cnt[ci];
          mine_cnt[ci] += val;
          if (!feasible(ci)) {
            ok = false;
          }
        }

        if (ok) {
          dfs(idx + 1);
        }

        for (int ci : var_to_cons[v]) {
          --assigned_cnt[ci];
          mine_cnt[ci] -= val;
        }
      }
      assign[v] = -1;
    };

    dfs(0);

    if (total_ways < 0.5L) {
      continue;
    }

    for (int li = 0; li < n; ++li) {
      double p = static_cast<double>(mine_ways[li] / total_ways);
      int gid = frontier_ids[vars[li]];
      auto [r, c] = DecodeId(gid);
      has_exact_prob[r][c] = true;
      exact_prob[r][c] = p;

      if (p <= 1e-12) {
        EnqueueVisit(r, c);
        changed = true;
      } else if (p >= 1.0 - 1e-12) {
        EnqueueMark(r, c);
        changed = true;
      }
    }
  }

  return changed;
}

Action ChooseGuess() {
  int unknown_cnt = 0;
  int flagged_cnt = 0;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] == kUnknown) {
        ++unknown_cnt;
      } else if (board[r][c] == kFlagged) {
        ++flagged_cnt;
      }
    }
  }

  double base_p = 0.5;
  if (unknown_cnt > 0) {
    int remain_mines = total_mines - flagged_cnt;
    if (remain_mines < 0) {
      remain_mines = 0;
    }
    if (remain_mines > unknown_cnt) {
      remain_mines = unknown_cnt;
    }
    base_p = static_cast<double>(remain_mines) / static_cast<double>(unknown_cnt);
  }

  bool found = false;
  double best_p = 2.0;
  int best_info = -1;
  int best_r = -1, best_c = -1;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] != kUnknown) {
        continue;
      }

      double p = base_p;
      int info = 0;

      if (has_exact_prob[r][c]) {
        p = exact_prob[r][c];
      } else {
        double sum_local = 0.0;
        int cnt_local = 0;
        for (int k = 0; k < 8; ++k) {
          int nr = r + dr[k], nc = c + dc[k];
          if (!InBounds(nr, nc) || board[nr][nc] < 0) {
            continue;
          }
          ++info;
          int clue = board[nr][nc];
          int flagged = 0, unknown = 0;
          for (int t = 0; t < 8; ++t) {
            int xr = nr + dr[t], xc = nc + dc[t];
            if (!InBounds(xr, xc)) {
              continue;
            }
            if (board[xr][xc] == kFlagged) {
              ++flagged;
            } else if (board[xr][xc] == kUnknown) {
              ++unknown;
            }
          }
          if (unknown > 0) {
            int need = clue - flagged;
            if (need < 0) {
              need = 0;
            }
            if (need > unknown) {
              need = unknown;
            }
            double local_p = static_cast<double>(need) / static_cast<double>(unknown);
            sum_local += local_p;
            ++cnt_local;
          }
        }
        if (cnt_local > 0) {
          p = (base_p + sum_local / cnt_local) * 0.5;
        }
      }

      if (!found || p < best_p - 1e-12 ||
          (std::abs(p - best_p) <= 1e-12 && info > best_info) ||
          (std::abs(p - best_p) <= 1e-12 && info == best_info &&
           std::abs(r - rows / 2) + std::abs(c - columns / 2) <
               std::abs(best_r - rows / 2) + std::abs(best_c - columns / 2))) {
        found = true;
        best_p = p;
        best_info = info;
        best_r = r;
        best_c = c;
      }
    }
  }

  if (!found) {
    return {0, 0, 0};
  }
  return {best_r, best_c, 0};
}
}  // namespace client_internal
using namespace client_internal;

/**
 * @brief The definition of function InitGame()
 *
 * @details This function is designed to initialize the game. It should be called at the beginning of the game, which
 * will read the scale of the game map and the first step taken by the server (see README).
 */
void InitGame() {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      board[i][j] = kUnknown;
      queued_visit[i][j] = false;
      queued_mark[i][j] = false;
      queued_auto[i][j] = false;
      has_exact_prob[i][j] = false;
      exact_prob[i][j] = 0.0;
    }
  }
  ResetQueues();
  ClearExactProb();

  int first_row, first_column;
  std::cin >> first_row >> first_column;
  Execute(first_row, first_column, 0);
}

/**
 * @brief The definition of function ReadMap()
 *
 * @details This function is designed to read the game map from stdin when playing the client's (or player's) role.
 * Since the client (or player) can only get the limited information of the game map, so if there is a 3 * 3 map as
 * above and only the block (2, 0) has been visited, the stdin would be
 *     ???
 *     12?
 *     01?
 */
void ReadMap() {
  for (int i = 0; i < rows; ++i) {
    std::string line;
    std::cin >> line;
    for (int j = 0; j < columns; ++j) {
      char ch = line[j];
      if (ch == '?') {
        board[i][j] = kUnknown;
      } else if (ch == '@') {
        board[i][j] = kFlagged;
      } else if ('0' <= ch && ch <= '8') {
        board[i][j] = ch - '0';
      } else {
        board[i][j] = kUnknown;
      }
    }
  }
}

/**
 * @brief The definition of function Decide()
 *
 * @details This function is designed to decide the next step when playing the client's (or player's) role. Open up your
 * mind and make your decision here! Caution: you can only execute once in this function.
 */
void Decide() {
  if (ExecuteFromQueue()) {
    return;
  }

  bool changed = true;
  int rounds = 0;
  while (changed && rounds < 8) {
    changed = false;
    ClearExactProb();

    if (ApplyBasicInference()) {
      changed = true;
    }
    auto constraints = BuildConstraints();
    if (ApplySubsetInference(constraints)) {
      changed = true;
    }
    if (ApplyExactComponentInference(constraints)) {
      changed = true;
    }
    ++rounds;
  }

  if (ExecuteFromQueue()) {
    return;
  }

  // Refresh exact probabilities for guessing stage.
  ClearExactProb();
  auto constraints = BuildConstraints();
  ApplyExactComponentInference(constraints);

  if (ExecuteFromQueue()) {
    return;
  }

  Action guess = ChooseGuess();
  Execute(guess.r, guess.c, guess.type);
}

#endif