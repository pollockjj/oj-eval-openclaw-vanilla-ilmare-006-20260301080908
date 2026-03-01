#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm>
#include <deque>
#include <iostream>
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

int board[kMaxN][kMaxN];
std::deque<Action> action_queue;
bool queued_visit[kMaxN][kMaxN];
bool queued_mark[kMaxN][kMaxN];
bool queued_auto[kMaxN][kMaxN];

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
  if (a.type == 0) {
    return board[a.r][a.c] == kUnknown;
  }
  if (a.type == 1) {
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

struct Equation {
  std::vector<int> cells;
  int need;
};

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

bool ApplySubsetInference() {
  std::vector<Equation> eqs;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      if (board[r][c] < 0) {
        continue;
      }
      int clue = board[r][c];
      int flagged = 0;
      std::vector<int> unknown_cells;
      for (int k = 0; k < 8; ++k) {
        int nr = r + dr[k], nc = c + dc[k];
        if (!InBounds(nr, nc)) {
          continue;
        }
        if (board[nr][nc] == kFlagged) {
          ++flagged;
        } else if (board[nr][nc] == kUnknown) {
          unknown_cells.push_back(CellId(nr, nc));
        }
      }
      int need = clue - flagged;
      if (!unknown_cells.empty() && 0 <= need && need <= static_cast<int>(unknown_cells.size())) {
        std::sort(unknown_cells.begin(), unknown_cells.end());
        unknown_cells.erase(std::unique(unknown_cells.begin(), unknown_cells.end()), unknown_cells.end());
        eqs.push_back({unknown_cells, need});
      }
    }
  }

  bool changed = false;
  int n = static_cast<int>(eqs.size());
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (i == j) {
        continue;
      }
      const auto &A = eqs[i].cells;
      const auto &B = eqs[j].cells;
      if (A.empty() || B.empty() || A == B) {
        continue;
      }
      if (!IsSubset(A, B)) {
        continue;
      }
      int need_diff = eqs[j].need - eqs[i].need;
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
          if (local_p > p) {
            p = local_p;
          }
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
    }
  }
  ResetQueues();

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
        // 'X' appears only when game is already over, but keep parser robust.
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
  while (changed && rounds < 6) {
    changed = false;
    if (ApplyBasicInference()) {
      changed = true;
    }
    if (ApplySubsetInference()) {
      changed = true;
    }
    ++rounds;
  }

  if (ExecuteFromQueue()) {
    return;
  }

  Action guess = ChooseGuess();
  Execute(guess.r, guess.c, guess.type);
}

#endif