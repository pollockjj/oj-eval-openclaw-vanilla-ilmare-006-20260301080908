#ifndef SERVER_H
#define SERVER_H

#include <cstdlib>
#include <iostream>
#include <queue>
#include <string>

/*
 * You may need to define some global variables for the information of the game map here.
 * Although we don't encourage to use global variables in real cpp projects, you may have to use them because the use of
 * class is not taught yet. However, if you are member of A-class or have learnt the use of cpp class, member functions,
 * etc., you're free to modify this structure.
 */
int rows;         // The count of rows of the game map. You MUST NOT modify its name.
int columns;      // The count of columns of the game map. You MUST NOT modify its name.
int total_mines;  // The count of mines of the game map. You MUST NOT modify its name. You should initialize this
                  // variable in function InitMap. It will be used in the advanced task.
int game_state;  // The state of the game, 0 for continuing, 1 for winning, -1 for losing. You MUST NOT modify its name.

// Internal states
namespace server_internal {
constexpr int kSrvMaxN = 35;
constexpr int kSrvUnknown = 0;
constexpr int kSrvVisited = 1;
constexpr int kSrvMarked = 2;

bool mine_map[kSrvMaxN][kSrvMaxN];
int visible_state[kSrvMaxN][kSrvMaxN];
int adjacent_mines[kSrvMaxN][kSrvMaxN];

int visited_safe_count = 0;

const int srv_dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
const int srv_dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

inline bool InBoundsSrv(int r, int c) {
  return 0 <= r && r < rows && 0 <= c && c < columns;
}

void TrySetWin() {
  if (visited_safe_count == rows * columns - total_mines) {
    game_state = 1;
  }
}

void RevealFlood(int sr, int sc) {
  std::queue<std::pair<int, int>> q;
  if (visible_state[sr][sc] != kSrvUnknown || mine_map[sr][sc]) {
    return;
  }
  visible_state[sr][sc] = kSrvVisited;
  ++visited_safe_count;
  q.push({sr, sc});

  while (!q.empty()) {
    auto [r, c] = q.front();
    q.pop();
    if (adjacent_mines[r][c] != 0) {
      continue;
    }
    for (int i = 0; i < 8; ++i) {
      int nr = r + srv_dr[i], nc = c + srv_dc[i];
      if (!InBoundsSrv(nr, nc)) {
        continue;
      }
      if (visible_state[nr][nc] != kSrvUnknown) {
        continue;
      }
      if (mine_map[nr][nc]) {
        continue;
      }
      visible_state[nr][nc] = kSrvVisited;
      ++visited_safe_count;
      if (adjacent_mines[nr][nc] == 0) {
        q.push({nr, nc});
      }
    }
  }
}

int CountCorrectMarkedMines() {
  int cnt = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      if (visible_state[i][j] == kSrvMarked && mine_map[i][j]) {
        ++cnt;
      }
    }
  }
  return cnt;
}
}  // namespace server_internal
using namespace server_internal;

/**
 * @brief The definition of function InitMap()
 *
 * @details This function is designed to read the initial map from stdin. For example, if there is a 3 * 3 map in which
 * mines are located at (0, 1) and (1, 2) (0-based), the stdin would be
 *     3 3
 *     .X.
 *     ...
 *     ..X
 * where X stands for a mine block and . stands for a normal block. After executing this function, your game map
 * would be initialized, with all the blocks unvisited.
 */
void InitMap() {
  std::cin >> rows >> columns;
  total_mines = 0;
  game_state = 0;
  visited_safe_count = 0;

  for (int i = 0; i < rows; ++i) {
    std::string s;
    std::cin >> s;
    for (int j = 0; j < columns; ++j) {
      mine_map[i][j] = (s[j] == 'X');
      visible_state[i][j] = kSrvUnknown;
      if (mine_map[i][j]) {
        ++total_mines;
      }
    }
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      if (mine_map[i][j]) {
        adjacent_mines[i][j] = -1;
        continue;
      }
      int cnt = 0;
      for (int k = 0; k < 8; ++k) {
        int ni = i + srv_dr[k], nj = j + srv_dc[k];
        if (InBoundsSrv(ni, nj) && mine_map[ni][nj]) {
          ++cnt;
        }
      }
      adjacent_mines[i][j] = cnt;
    }
  }
}

/**
 * @brief The definition of function VisitBlock(int, int)
 *
 * @details This function is designed to visit a block in the game map. We take the 3 * 3 game map above as an example.
 * At the beginning, if you call VisitBlock(0, 0), the return value would be 0 (game continues), and the game map would
 * be
 *     1??
 *     ???
 *     ???
 * If you call VisitBlock(0, 1) after that, the return value would be -1 (game ends and the players loses) , and the
 * game map would be
 *     1X?
 *     ???
 *     ???
 * If you call VisitBlock(0, 2), VisitBlock(2, 0), VisitBlock(1, 2) instead, the return value of the last operation
 * would be 1 (game ends and the player wins), and the game map would be
 *     1@1
 *     122
 *     01@
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 *
 * @note You should edit the value of game_state in this function. Precisely, edit it to
 *    0  if the game continues after visit that block, or that block has already been visited before.
 *    1  if the game ends and the player wins.
 *    -1 if the game ends and the player loses.
 *
 * @note For invalid operation, you should not do anything.
 */
void VisitBlock(int r, int c) {
  if (!InBoundsSrv(r, c)) {
    return;
  }
  if (visible_state[r][c] == kSrvVisited || visible_state[r][c] == kSrvMarked) {
    return;
  }

  if (mine_map[r][c]) {
    visible_state[r][c] = kSrvVisited;
    game_state = -1;
    return;
  }

  RevealFlood(r, c);
  TrySetWin();
}

/**
 * @brief The definition of function MarkMine(int, int)
 *
 * @details This function is designed to mark a mine in the game map.
 * If the block being marked is a mine, show it as "@".
 * If the block being marked isn't a mine, END THE GAME immediately. (NOTE: This is not the same rule as the real
 * game) And you don't need to
 *
 * For example, if we use the same map as before, and the current state is:
 *     1?1
 *     ???
 *     ???
 * If you call MarkMine(0, 1), you marked the right mine. Then the resulting game map is:
 *     1@1
 *     ???
 *     ???
 * If you call MarkMine(1, 0), you marked the wrong mine(There's no mine in grid (1, 0)).
 * The game_state would be -1 and game ends immediately. The game map would be:
 *     1?1
 *     X??
 *     ???
 * This is different from the Minesweeper you've played. You should beware of that.
 *
 * @param r The row coordinate (0-based) of the block to be marked.
 * @param c The column coordinate (0-based) of the block to be marked.
 *
 * @note You should edit the value of game_state in this function. Precisely, edit it to
 *    0  if the game continues after visit that block, or that block has already been visited before.
 *    1  if the game ends and the player wins.
 *    -1 if the game ends and the player loses.
 *
 * @note For invalid operation, you should not do anything.
 */
void MarkMine(int r, int c) {
  if (!InBoundsSrv(r, c)) {
    return;
  }
  if (visible_state[r][c] == kSrvVisited || visible_state[r][c] == kSrvMarked) {
    return;
  }

  visible_state[r][c] = kSrvMarked;
  if (!mine_map[r][c]) {
    game_state = -1;
    return;
  }

  TrySetWin();
}

/**
 * @brief The definition of function AutoExplore(int, int)
 *
 * @details This function is designed to auto-visit adjacent blocks of a certain block.
 * See README.md for more information
 *
 * For example, if we use the same map as before, and the current map is:
 *     ?@?
 *     ?2?
 *     ??@
 * Then auto explore is available only for block (1, 1). If you call AutoExplore(1, 1), the resulting map will be:
 *     1@1
 *     122
 *     01@
 * And the game ends (and player wins).
 */
void AutoExplore(int r, int c) {
  if (!InBoundsSrv(r, c)) {
    return;
  }
  if (visible_state[r][c] != kSrvVisited) {
    return;
  }
  if (mine_map[r][c]) {
    return;
  }

  int marked_neighbors = 0;
  for (int i = 0; i < 8; ++i) {
    int nr = r + srv_dr[i], nc = c + srv_dc[i];
    if (!InBoundsSrv(nr, nc)) {
      continue;
    }
    if (visible_state[nr][nc] == kSrvMarked) {
      ++marked_neighbors;
    }
  }

  if (marked_neighbors != adjacent_mines[r][c]) {
    return;
  }

  for (int i = 0; i < 8; ++i) {
    int nr = r + srv_dr[i], nc = c + srv_dc[i];
    if (!InBoundsSrv(nr, nc)) {
      continue;
    }
    if (visible_state[nr][nc] == kSrvMarked) {
      continue;
    }
    if (visible_state[nr][nc] == kSrvVisited) {
      continue;
    }
    VisitBlock(nr, nc);
    if (game_state == -1) {
      return;
    }
  }

  if (game_state == 0) {
    TrySetWin();
  }
}

/**
 * @brief The definition of function ExitGame()
 *
 * @details This function is designed to exit the game.
 * It outputs a line according to the result, and a line of two integers, visit_count and marked_mine_count,
 * representing the number of blocks visited and the number of marked mines taken respectively.
 *
 * @note If the player wins, we consider that ALL mines are correctly marked.
 */
void ExitGame() {
  if (game_state == 1) {
    std::cout << "YOU WIN!" << std::endl;
    std::cout << visited_safe_count << " " << total_mines << std::endl;
  } else {
    std::cout << "GAME OVER!" << std::endl;
    std::cout << visited_safe_count << " " << CountCorrectMarkedMines() << std::endl;
  }
  exit(0);  // Exit the game immediately
}

/**
 * @brief The definition of function PrintMap()
 *
 * @details This function is designed to print the game map to stdout. We take the 3 * 3 game map above as an example.
 * At the beginning, if you call PrintMap(), the stdout would be
 *    ???
 *    ???
 *    ???
 * If you call VisitBlock(2, 0) and PrintMap() after that, the stdout would be
 *    ???
 *    12?
 *    01?
 * If you call VisitBlock(0, 1) and PrintMap() after that, the stdout would be
 *    ?X?
 *    12?
 *    01?
 * If the player visits all blocks without mine and call PrintMap() after that, the stdout would be
 *    1@1
 *    122
 *    01@
 * (You may find the global variable game_state useful when implementing this function.)
 *
 * @note Use std::cout to print the game map, especially when you want to try the advanced task!!!
 */
void PrintMap() {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      char ch = '?';
      if (game_state == 1 && mine_map[i][j]) {
        ch = '@';
      } else if (visible_state[i][j] == kSrvUnknown) {
        ch = '?';
      } else if (visible_state[i][j] == kSrvMarked) {
        ch = mine_map[i][j] ? '@' : 'X';
      } else {  // visited
        ch = mine_map[i][j] ? 'X' : static_cast<char>('0' + adjacent_mines[i][j]);
      }
      std::cout << ch;
    }
    std::cout << std::endl;
  }
}

#endif