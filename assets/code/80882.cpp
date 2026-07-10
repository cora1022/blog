#include <algorithm>
  #include <cassert>
  #include <cmath>
  #include <cstdlib>
  #include <iostream>
  #include <limits>
  #include <sstream>
  #include <string>
  #include <utility>
  #include <vector>


  constexpr int MAX_TURN = 200;
  constexpr int START_GOLD = 500;
  constexpr int START_WARRIORS = 3;
  constexpr int MOVE_COST = 10;
  constexpr int TRAIN_COST = 120;
  constexpr int WORK_INCOME = 15;
  constexpr int UPKEEP_PER_WARRIOR = 2;
  constexpr int HQ_MAX_LEVEL = 5;
  constexpr int BASE_MAX_LEVEL = 3;
  constexpr int HQ_HEAL_COST = 1000;
  constexpr int BASE_HEAL_COST = 500;

  struct HqLevelEntry { int upgrade_cost; int warrior_hp; int hp; int turret; int train_cap; int work_cap; };
  struct BaseLevelEntry { int cost; int hp; int turret; int work_cap; };

  constexpr HqLevelEntry HQ_LEVELS[HQ_MAX_LEVEL + 1] = {
      {0,0,0,0,0,0},{0,4,10,1,1,1},{600,5,15,2,1,2},
      {1200,6,20,2,2,3},{2400,7,25,3,2,4},{3600,8,30,3,3,5},};

  constexpr BaseLevelEntry BASE_LEVELS[BASE_MAX_LEVEL + 1] = {
      {0,0,0,0},{300,6,1,1},{600,12,1,2},{1000,18,2,3},};

  enum class Side : int { LEFT = 0, RIGHT = 1 };
  enum class BType : int { HQ, BASE };
  enum class WState : int { STATIONARY, MOVING };
  inline Side opposite(Side s) { return s == Side::LEFT ? Side::RIGHT : Side::LEFT; }
  inline char side_char(Side s) { return s == Side::LEFT ? 'A' : 'B'; }
  inline Side parse_side_char(char c) { return c == 'A' ? Side::LEFT : Side::RIGHT; }
  struct WarriorId {
    Side side = Side::LEFT; int num = 0;
    WarriorId() = default;
    WarriorId(Side s, int n) : side(s), num(n) {}
    bool operator==(const WarriorId &o) const { return side == o.side && num == o.num; }
  };

  struct Warrior { WarriorId id; int region = 0; int hp = 0; WState state = WState::STATIONARY; int target = 0; };
  struct Building {
    int region = 0; Side side = Side::LEFT; BType type = BType::HQ; int level = 1; int hp = 10;
    Building() = default;
    Building(int region_, Side side_, BType type_, int level_, int hp_) : region(region_), side(side_), type(type_), level(level_), hp(hp_) {}
    int current_hp() const { return type == BType::HQ ? HQ_LEVELS[level].hp : BASE_LEVELS[level].hp; }
    int work_cap() const { return type == BType::HQ ? HQ_LEVELS[level].work_cap : BASE_LEVELS[level].work_cap; }
  };
  struct GameMap {
    int N = 0, K = 0; std::vector<long long> x, y; std::vector<int> strongholds;
    std::vector<std::vector<int>> adj; Side my_side = Side::LEFT; int my_hq = 0; int opp_hq = 0;
  };
  struct GameState {
    int gold = START_GOLD; int my_countdown = 5; int opp_countdown = 5;
    std::vector<Warrior> warriors; std::vector<Building> buildings;
  };
  struct Actions { int train_n = 0; std::vector<std::pair<WarriorId, int>> moves; std::vector<int> upgrades; };
  static std::string readln() { std::string s; if (!std::getline(std::cin, s)) std::exit(0); return s; }
  static std::vector<std::string> tokens(const std::string &s) {
    std::vector<std::string> out; std::istringstream is(s);
    for (std::string t; is >> t;) { out.push_back(t); }
    return out;
  }
  static WarriorId parse_warrior(const std::string &tok) {
    assert(!tok.empty() && (tok[0] == 'A' || tok[0] == 'B'));
    WarriorId id; id.side = parse_side_char(tok[0]); id.num = std::stoi(tok.substr(1)); return id;
  }
  static std::string format_warrior(WarriorId id) {
    std::string s; s.push_back(side_char(id.side)); s += std::to_string(id.num); return s;
  }
  static int hq_of(const GameMap &M, Side s) { return (s == Side::LEFT) ? 0 : M.N - 1; }
  static Building make_base(int region, Side s) { return Building(region, s, BType::BASE, 1, BASE_LEVELS[1].hp); }
  static void apply_upgrade(Building &b) { b.level += 1; b.hp = b.current_hp(); }
  static int upgrade_cost(const Building &b) {
    if (b.type == BType::HQ) return HQ_LEVELS[b.level + 1].upgrade_cost;
    else return BASE_LEVELS[b.level + 1].cost;
  }
  static int max_level(const Building &b) { return b.type == BType::HQ ? HQ_MAX_LEVEL : BASE_MAX_LEVEL; }

  static void parse_init(GameMap &M, GameState &S) {
    { auto t = tokens(readln()); assert(t.size() >= 2 && t[0] == "READY"); M.my_side = (t[1] == "LEFT") ? Side::LEFT : Side::RIGHT; }
    { auto t = tokens(readln()); M.N = std::stoi(t.at(0)); M.K = std::stoi(t.at(1)); }
    M.x.assign(M.N, 0); M.y.assign(M.N, 0);
    { auto t = tokens(readln()); for (int i = 0; i < M.N; ++i) M.x[i] = std::stoll(t.at(i)); }
    { auto t = tokens(readln()); for (int i = 0; i < M.N; ++i) M.y[i] = std::stoll(t.at(i)); }
    { auto t = tokens(readln()); M.strongholds.clear(); M.strongholds.reserve(t.size());
      for (const auto &s : t) M.strongholds.push_back(std::stoi(s));
      std::sort(M.strongholds.begin(), M.strongholds.end()); }
    M.adj.assign(M.N, {});
    for (int r = 0; r < M.N; ++r) {
      auto t = tokens(readln()); int deg = std::stoi(t.at(0)); auto &nb = M.adj[r]; nb.reserve(deg);
      for (int j = 0; j < deg; ++j) nb.push_back(std::stoi(t.at(1 + j)));
      std::sort(nb.begin(), nb.end());
    }
    M.my_hq = hq_of(M, M.my_side); M.opp_hq = hq_of(M, opposite(M.my_side));
    S = GameState{}; S.gold = START_GOLD; Side opp = opposite(M.my_side);
    for (int sfx = 1; sfx <= START_WARRIORS; ++sfx) {
      Warrior w1; w1.id = WarriorId(M.my_side, sfx); w1.region = M.my_hq; w1.hp = HQ_LEVELS[1].warrior_hp; S.warriors.push_back(w1);
      Warrior w2; w2.id = WarriorId(opp, sfx); w2.region = M.opp_hq; w2.hp = HQ_LEVELS[1].warrior_hp; S.warriors.push_back(w2);
    }
    S.buildings.push_back(Building(hq_of(M, Side::LEFT), Side::LEFT, BType::HQ, 1, HQ_LEVELS[1].hp));
    S.buildings.push_back(Building(hq_of(M, Side::RIGHT), Side::RIGHT, BType::HQ, 1, HQ_LEVELS[1].hp));
    std::cout << "OK" << std::endl;
  }
  static bool read_turn_start(int &turn_index) {
    std::string line = readln(); if (line == "FINISH") return false;
    auto t = tokens(line); assert(!t.empty() && t[0] == "START"); turn_index = std::stoi(t.at(2)); return true;
  }
  static Building *find_building(GameState &S, int region) {
    for (auto &b : S.buildings) { if (b.region == region) return &b; }
    return nullptr;
  }
  static Warrior *find_warrior(GameState &S, WarriorId id) {
    for (auto &w : S.warriors) { if (w.id == id) return &w; }
    return nullptr;
  }


  static void read_turn_result(GameState &S, const GameMap &M, const Actions &submitted) {
    for (int region : submitted.upgrades) {
      Building *b = find_building(S, region);
      if (b == nullptr) { S.gold -= BASE_LEVELS[1].cost; S.buildings.push_back(make_base(region, M.my_side)); }
      else { if (b->level >= max_level(*b)) { int cost = (b->type == BType::HQ) ? HQ_HEAL_COST : BASE_HEAL_COST; S.gold -= cost; b->hp = b->current_hp(); }
        else { S.gold -= upgrade_cost(*b); apply_upgrade(*b); } }
    }
    for (const std::pair<WarriorId, int> &mv : submitted.moves) {
      const WarriorId &id = mv.first; int target = mv.second;
      Building *b = find_building(S, target);
      int cost = (b != nullptr && b->side == M.my_side) ? 0 : MOVE_COST; S.gold -= cost;
      if (Warrior *w = find_warrior(S, id)) { w->state = WState::MOVING; w->target = target; }
    }
    S.gold -= TRAIN_COST * submitted.train_n;
    { std::string line = readln(); if (line == "FINISH") std::exit(0); auto t = tokens(line); assert(!t.empty() && t[0] == "TURN"); }
    { auto t = tokens(readln()); S.my_countdown = std::stoi(t.at(2)); S.opp_countdown = std::stoi(t.at(4)); }
    { auto t = tokens(readln()); int n = std::stoi(t.at(1));
      for (int i = 0; i < n; ++i) { auto r = tokens(readln()); Side s = parse_side_char(r.at(0)[0]); int region = std::stoi(r.at(1));
        Building *b = find_building(S, region);
        if (b == nullptr) { S.buildings.push_back(make_base(region, s)); }
        else if (b->side != M.my_side) { if (b->level >= max_level(*b)) { b->hp = b->current_hp(); } else { apply_upgrade(*b); } } } }
    { auto t = tokens(readln()); int n = std::stoi(t.at(1));
      if (n > 0) { auto ids = tokens(readln());
        for (int i = 0; i < n; ++i) { WarriorId id = parse_warrior(ids.at(i)); int hq_region = hq_of(M, id.side);
          Building *hq_b = find_building(S, hq_region); int hq_level = (hq_b != nullptr) ? hq_b->level : 1;
          Warrior w; w.id = id; w.region = hq_region; w.hp = HQ_LEVELS[hq_level].warrior_hp; S.warriors.push_back(w); } } }
    { auto t = tokens(readln()); int n = std::stoi(t.at(1));
      for (int i = 0; i < n; ++i) { auto r = tokens(readln()); WarriorId id = parse_warrior(r.at(0)); int region = std::stoi(r.at(1));
        if (Warrior *w = find_warrior(S, id)) { w->region = region;
          if (id.side == M.my_side && w->state == WState::MOVING && w->region == w->target) { w->state = WState::STATIONARY; } } } }
    { auto t = tokens(readln()); int n = std::stoi(t.at(1));
      for (int i = 0; i < n; ++i) { auto r = tokens(readln()); WarriorId id = parse_warrior(r.at(1)); int damage = std::stoi(r.at(2));
        if (Warrior *w = find_warrior(S, id)) w->hp -= damage; }
      S.warriors.erase(std::remove_if(S.warriors.begin(), S.warriors.end(), [](const Warrior &w) { return w.hp <= 0; }), S.warriors.end()); }
    { auto t = tokens(readln()); int n = std::stoi(t.at(1));
      for (int i = 0; i < n; ++i) { auto r = tokens(readln()); int region = std::stoi(r.at(1)); int damage = std::stoi(r.at(2));
        if (Building *b = find_building(S, region)) b->hp -= damage; }
      S.buildings.erase(std::remove_if(S.buildings.begin(), S.buildings.end(), [](const Building &b) { return b.hp <= 0; }), S.buildings.end()); }
    (void)readln();
    int income = 0;
    for (const auto &b : S.buildings) { if (b.side != M.my_side) continue; int count = 0;
      for (const auto &w : S.warriors) { if (w.id.side == M.my_side && w.region == b.region) ++count; }
      income += WORK_INCOME * std::min(count, b.work_cap()); }
    S.gold += income;
    int alive = 0; for (const auto &w : S.warriors) if (w.id.side == M.my_side) ++alive;
    S.gold = std::max(0, S.gold - UPKEEP_PER_WARRIOR * alive);
  }
  struct Paths { std::vector<std::vector<double>> dist; std::vector<std::vector<int>> nxt; };
  static double euclid_ceil(const GameMap &M, int u, int v) {
    double dx = (double)(M.x[u] - M.x[v]); double dy = (double)(M.y[u] - M.y[v]);
    return std::ceil(std::sqrt(dx * dx + dy * dy));
  }

  static Paths calculate_paths(const GameMap &M) {
    const double INF = std::numeric_limits<double>::infinity(); Paths P;
    P.dist.assign(M.N, std::vector<double>(M.N, INF)); P.nxt.assign(M.N, std::vector<int>(M.N, -1));
    for (int i = 0; i < M.N; ++i) { P.dist[i][i] = 0.0; P.nxt[i][i] = i; }
    for (int u = 0; u < M.N; ++u) for (int v : M.adj[u]) { double w = euclid_ceil(M, u, v); if (w < P.dist[u][v]) P.dist[u][v] = w; }
    for (int k = 0; k < M.N; ++k) for (int u = 0; u < M.N; ++u) { if (P.dist[u][k] == INF) continue;
      for (int v = 0; v < M.N; ++v) { double cand = P.dist[u][k] + P.dist[k][v]; if (cand < P.dist[u][v]) P.dist[u][v] = cand; } }
    for (int u = 0; u < M.N; ++u) for (int v = 0; v < M.N; ++v) { if (u == v || P.dist[u][v] == INF) continue;
      double best_score = INF; for (int nb : M.adj[u]) { if (P.dist[nb][v] == INF) continue;
        double score = euclid_ceil(M, u, nb) + P.dist[nb][v]; if (score < best_score) { best_score = score; P.nxt[u][v] = nb; } } }
    return P;
  }
  static void emit_command() { std::cout << "COMMAND\n"; }
  static void emit_actions(const Actions &a) {
    for (const std::pair<WarriorId, int> &mv : a.moves) std::cout << "MOVE " << format_warrior(mv.first) << ' ' << mv.second << '\n';
    for (int r : a.upgrades) std::cout << "UPGRADE " << r << '\n';
    if (a.train_n > 0) std::cout << "TRAIN " << a.train_n << '\n';
  }
  static void emit_end() { std::cout << "END" << std::endl; }


  static const Building *bat(const GameState &S, int region) {
    for (const auto &b : S.buildings) if (b.region == region) return &b;
    return nullptr;
  }
  static int my_cnt(const GameState &S, Side me, int r) {
    int c = 0; for (const auto &w : S.warriors) if (w.id.side == me && w.region == r && w.hp > 0) ++c; return c;
  }
  static int en_cnt(const GameState &S, Side me, int r) {
    int c = 0; for (const auto &w : S.warriors) if (w.id.side != me && w.region == r && w.hp > 0) ++c; return c;
  }
  static bool is_sh(const GameMap &M, int r) { return std::binary_search(M.strongholds.begin(), M.strongholds.end(), r); }
  static bool my_side_region(const GameMap &M, const Paths &P, int r) { return P.dist[M.my_hq][r] <= P.dist[M.opp_hq][r]; }
  static int turret_of(const Building &b) { return b.type == BType::HQ ? HQ_LEVELS[b.level].turret : BASE_LEVELS[b.level].turret; }

  struct SimResult { bool win = false; int survivors = 0; };
  static int weakest(const std::vector<int> &h) {
    int bi = -1; for (int i = 0; i < (int)h.size(); ++i) if (h[i] > 0 && (bi < 0 || h[i] < h[bi])) bi = i; return bi;
  }


  static SimResult sim_siege(std::vector<int> atk, std::vector<int> def, int bhp, int tur, int maxd = 100) {
    for (int d = 0; d < maxd; ++d) {
      int a0 = 0; for (int h : atk) if (h > 0) ++a0;
      int b0 = 0; for (int h : def) if (h > 0) ++b0;
      if (a0 == 0) return {false, 0};
      for (int t = 0; t < tur; ++t) { int i = weakest(atk); if (i < 0) break; atk[i]--; }
      for (int t = 0; t < b0; ++t) { int i = weakest(atk); if (i < 0) break; atk[i]--; }
      for (int t = 0; t < a0; ++t) { int i = weakest(def); if (i >= 0) def[i]--; else if (bhp > 0) bhp--; }
      if (bhp <= 0) { int s = 0; for (int h : atk) if (h > 0) ++s; return {true, s}; }
      bool any = false; for (int h : atk) if (h > 0) { any = true; break; }
      if (!any) return {false, 0};
    }
    return {false, 0};
  }


  struct Persist { bool inited = false; int rally = -1; int attack_target = -1; bool emergency = false; bool rush = false; bool tempo = false; int def_node = -1; int center_sent = 0; };
  static Persist G;


  static Actions decide(const GameState &S, const GameMap &M, const Paths &P, int turn) {
    Actions a;
    const Side me = M.my_side;
    int budget = S.gold;
    if (!G.inited) { G.inited = true; G.rally = (M.N - 1) / 2; G.center_sent = 0; }
    const int central = G.rally;
    const Building *myhq = bat(S, M.my_hq);
    int hqlv = myhq ? myhq->level : 1;
    int whp = HQ_LEVELS[hqlv].warrior_hp;

    std::vector<const Building *> mybld, enbld;
    for (const auto &b : S.buildings) (b.side == me ? mybld : enbld).push_back(&b);
    int bases = 0; for (auto b : mybld) if (b->type == BType::BASE) ++bases;

    std::vector<const Warrior *> mine;
    for (const auto &w : S.warriors) if (w.id.side == me && w.hp > 0) mine.push_back(&w);
    int ntot = (int)mine.size();


    std::vector<char> worker(mine.size(), 0);
    for (auto b : mybld) {
      std::vector<int> here;
      for (int i = 0; i < (int)mine.size(); ++i) if (mine[i]->region == b->region) here.push_back(i);
      std::sort(here.begin(), here.end(), [&](int x, int y) { return mine[x]->id.num < mine[y]->id.num; });
      for (int k = 0; k < (int)here.size() && k < b->work_cap(); ++k) worker[here[k]] = 1;
    }


    auto omove = [&](const Warrior &w, int dest) -> bool {
      if (w.state == WState::MOVING) return false;
      for (const auto &mv : a.moves) if (mv.first == w.id) return false;
      if (dest < 0 || dest >= M.N || w.region == dest) return false;
      const Building *b = bat(S, dest);
      int cost = (b && b->side == me) ? 0 : MOVE_COST;
      if (budget < cost) return false;
      budget -= cost; a.moves.emplace_back(w.id, dest); return true;
    };
    auto owned = [&](int r) { const Building *b = bat(S, r); return b && b->side == me; };

    auto def_hp = [&](int r) -> std::vector<int> {
      int rad = (r == M.opp_hq) ? 2 : 1;
      std::vector<char> near(M.N, 0); near[r] = 1;
      std::vector<int> frontier = {r};
      for (int h = 0; h < rad; ++h) {
        std::vector<int> nxt;
        for (int x : frontier) for (int nb : M.adj[x]) if (!near[nb]) { near[nb] = 1; nxt.push_back(nb); }
        frontier.swap(nxt);
      }
      std::vector<int> v;
      for (const auto &w : S.warriors) if (w.id.side != me && w.hp > 0 && near[w.region]) v.push_back(w.hp);
      std::sort(v.begin(), v.end());
      return v;
    };

    auto needed_fresh = [&](int r) -> int {
      const Building *b = bat(S, r); int hp = b ? b->hp : 1; int tur = b ? turret_of(*b) : 0;
      std::vector<int> def = def_hp(r);
      for (int k = 1; k <= 60; ++k) { std::vector<int> f(k, whp); if (sim_siege(f, def, hp, tur).win) return k; }
      return 999;
    };

    std::vector<int> army;
    for (int i = 0; i < (int)mine.size(); ++i) if (!worker[i]) army.push_back(i);


    std::vector<int> home_threat;
    for (const auto &w : S.warriors) if (w.id.side != me && w.hp > 0 && P.dist[M.my_hq][w.region] < P.dist[M.opp_hq][w.region]) home_threat.push_back(w.hp);
    std::sort(home_threat.begin(), home_threat.end());
    int hqhp0 = myhq ? myhq->hp : HQ_LEVELS[1].hp;
    int hqtur0 = myhq ? turret_of(*myhq) : HQ_LEVELS[1].turret;
    int guard_need;
    if (home_threat.empty()) { guard_need = (ntot >= 8) ? 2 : 0; }
    else { guard_need = 1; for (int g = 0; g <= 30; ++g) { std::vector<int> gd(g, whp); if (!sim_siege(home_threat, gd, hqhp0, hqtur0).win) { guard_need = std::max(1, g); break; } if (g == 30) guard_need = 30; } }
    std::vector<int> army_by_hq = army;
    std::sort(army_by_hq.begin(), army_by_hq.end(), [&](int x, int y) { return P.dist[mine[x]->region][M.my_hq] < P.dist[mine[y]->region][M.my_hq]; });
    if (home_threat.empty()) guard_need = std::min(guard_need, std::max(0, (int)army_by_hq.size() - 2));


    bool hq_alert = false;
    {
      std::vector<char> nearHQ(M.N, 0); nearHQ[M.my_hq] = 1; std::vector<int> fr = {M.my_hq};
      for (int h = 0; h < 5; ++h) { std::vector<int> nx; for (int x : fr) for (int nb : M.adj[x]) if (!nearHQ[nb]) { nearHQ[nb] = 1; nx.push_back(nb); } fr.swap(nx); }
      std::vector<int> approach;
      for (const auto &w : S.warriors) { if (w.id.side == me || w.hp <= 0) continue; if (!nearHQ[w.region]) continue;
        const Building *hb = bat(S, w.region); if (hb && hb->side != me) continue;
        approach.push_back(w.hp); }
      if ((int)approach.size() >= 3) {
        std::sort(approach.begin(), approach.end());
        int g2 = 1; for (int g = 0; g <= 30; ++g) { std::vector<int> gd(g, whp); if (!sim_siege(approach, gd, hqhp0, hqtur0).win) { g2 = std::max(1, g); break; } if (g == 30) g2 = 30; }
        guard_need = std::max(guard_need, std::min(g2, (int)army_by_hq.size()));
      }
      if ((int)approach.size() >= 4) hq_alert = true;
    }
    std::vector<char> is_reserve(mine.size(), 0);
    for (int k = 0; k < (int)army_by_hq.size() && k < guard_need; ++k) is_reserve[army_by_hq[k]] = 1;


    const Building *ehq = bat(S, M.opp_hq); int elv = ehq ? ehq->level : 1;
    bool need_up = myhq && elv > myhq->level && myhq->level < HQ_MAX_LEVEL;


    int defense_node = -1, def_need = 0; bool hq_emergency = false;
    {
      int reg_node = -1, reg_need = 0, reg_over = 0, hq_need = 0; bool hq_threat = false;
      int lock_over = 0, lock_need = 0;
      for (auto b : mybld) {
        int z = b->region; bool isHQ = (b->type == BType::HQ);
        std::vector<char> near(M.N, 0); near[z] = 1; std::vector<int> fr = {z};
        int rad = isHQ ? 3 : 2;
        for (int h = 0; h < rad; ++h) { std::vector<int> nx; for (int x : fr) for (int nb : M.adj[x]) if (!near[nb]) { near[nb] = 1; nx.push_back(nb); } fr.swap(nx); }
        int near_cnt = 0;
        for (const auto &w : S.warriors) { if (w.id.side == me || w.hp <= 0) continue; int wz = w.region; if (wz < 0 || wz >= M.N) continue;
          const Building *hb = bat(S, wz); if (hb && hb->side != me) continue; if (!near[wz]) continue;

          int onb = -1; double ond = 1e18; for (auto ob : mybld) { double d = P.dist[ob->region][wz]; if (d < ond) { ond = d; onb = ob->region; } }
          if (onb == z) ++near_cnt; }
        if (near_cnt == 0) continue;
        int tur = turret_of(*b), garrison = my_cnt(S, me, z);
        int over = near_cnt - (tur + garrison);
        if (over <= 0) continue;
        int need = (near_cnt - tur) + 1;
        if (isHQ) { hq_threat = true; hq_need = std::max(hq_need, need); }
        else if (need <= std::max(3, (int)army.size())) {
          if (over > reg_over) { reg_over = over; reg_node = z; reg_need = need; }
          if (z == G.def_node) { lock_over = over; lock_need = need; }
        }
      }

      if (!hq_threat && lock_over > 0 && reg_over <= lock_over + 1) { reg_node = G.def_node; reg_need = lock_need; }
      if (hq_threat) { defense_node = M.my_hq; def_need = hq_need; hq_emergency = true; }
      else if (reg_node >= 0) { defense_node = reg_node; def_need = reg_need; }
    }
    G.def_node = (defense_node >= 0 && !hq_emergency) ? defense_node : -1;


    std::vector<int> apts;
    for (auto b : enbld) apts.push_back(b->region);
    for (int s : M.strongholds) if (!owned(s) && bat(S, s) == nullptr && en_cnt(S, me, s) > 0) apts.push_back(s);
    int objective = -1, obj_need = 999; double obj_d = 1e18;
    for (int r : apts) { int nf = needed_fresh(r); double d = P.dist[central][r]; if (nf < obj_need || (nf == obj_need && d < obj_d)) { obj_need = nf; obj_d = d; objective = r; } }
    if (objective < 0) { objective = M.opp_hq; obj_need = needed_fresh(M.opp_hq); }

    bool only_hq = true; for (auto b : enbld) if (b->region != M.opp_hq) only_hq = false;
    bool endgame = only_hq && owned(central) && bases >= 3;


    int enemy_bases = 0; for (auto b : enbld) if (b->type == BType::BASE) ++enemy_bases;
    int enemy_war = 0; for (const auto &w : S.warriors) if (w.id.side != me && w.hp > 0) ++enemy_war;
    int biggest_clump = 0;
    { std::vector<int> cnt(M.N, 0);
      for (const auto &w : S.warriors) if (w.id.side != me && w.hp > 0) { if (++cnt[w.region] > biggest_clump) biggest_clump = cnt[w.region]; } }
    if ((turn <= 15) && (enemy_bases == 0) && (biggest_clump >= 5)) G.rush = true;
    if (enemy_bases > 0 || (home_threat.empty() && (int)mine.size() >= enemy_war)) G.rush = false;
    bool rush = G.rush;

    if (ntot + 3 < enemy_war) G.tempo = true;
    if (ntot >= enemy_war)   G.tempo = false;
    bool behind = G.tempo;
    bool stop_expand = behind && bases >= 2;


    int center_clump = 0;
    {
      std::vector<int> anchors; anchors.push_back(central); anchors.push_back(M.my_hq);
      for (auto b : mybld) anchors.push_back(b->region);
      for (int anc : anchors) {
        if (anc < 0 || anc >= M.N) continue;
        std::vector<char> nc(M.N, 0); nc[anc] = 1; std::vector<int> fr = {anc};
        for (int h = 0; h < 3; ++h) { std::vector<int> nx; for (int x : fr) for (int nb : M.adj[x]) if (!nc[nb]) { nc[nb] = 1; nx.push_back(nb); } fr.swap(nx); }
        int c = 0;
        for (const auto &w : S.warriors) { if (w.id.side == me || w.hp <= 0) continue; const Building *hb = bat(S, w.region); if (hb && hb->side != me) continue; if (nc[w.region]) ++c; }
        if (c > center_clump) center_clump = c;
      }
    }
    bool invasion = center_clump >= 4;
    int frontyard = -1; { double fd = 1e18;
      for (int s : M.strongholds) if (my_side_region(M, P, s) && !owned(s) && bat(S, s) == nullptr && en_cnt(S, me, s) == 0) { double d = P.dist[M.my_hq][s]; if (d < fd) { fd = d; frontyard = s; } } }


    int staging = M.my_hq; double sd = P.dist[M.my_hq][M.opp_hq];
    for (auto b : mybld) { double d = P.dist[b->region][M.opp_hq]; if (d < sd) { sd = d; staging = b->region; } }
    int rally_to = rush ? M.my_hq : (defense_node >= 0 ? defense_node : (hq_alert ? M.my_hq : staging));

    std::vector<int> muster;
    for (int i : army) if (!is_reserve[i] && mine[i]->region == staging && mine[i]->state == WState::STATIONARY) muster.push_back(i);
    int nm = (int)muster.size();

    int target = -1;
    if (nm >= needed_fresh(M.opp_hq) + 2) target = M.opp_hq;
    if (target < 0) { double best = 1e18;
      for (int r : apts) { if (r == M.opp_hq) continue; if (nm < needed_fresh(r) + 1) continue;
        double score = P.dist[r][M.opp_hq] * 1e4 + P.dist[staging][r]; if (score < best) { best = score; target = r; } } }
    if (rush) target = -1;
    G.attack_target = target;

    int income_est = 0; for (auto b : mybld) income_est += WORK_INCOME * b->work_cap();


    int hq_cost = (myhq && myhq->level < HQ_MAX_LEVEL) ? HQ_LEVELS[myhq->level + 1].upgrade_cost : 0;
    bool behind_lv = myhq && myhq->level < elv;
    bool hq_safe = myhq && myhq->level < HQ_MAX_LEVEL && en_cnt(S, me, M.my_hq) == 0 && my_cnt(S, me, M.my_hq) > 0
                  && defense_node < 0 && !rush && !behind && !invasion;
    bool want_hq_up = hq_safe && hq_cost > 0 && (behind_lv || (bases >= 2 && hq_cost <= income_est * 18));


    std::vector<char> committed(mine.size(), 0);
    int def_short = 0;
    if (defense_node >= 0) {
      if (hq_emergency) {

        int producible = std::min(HQ_LEVELS[hqlv].train_cap, budget / TRAIN_COST);
        std::vector<int> dh;
        for (int i = 0; i < (int)mine.size(); ++i) if (mine[i]->state == WState::STATIONARY && mine[i]->region == M.my_hq) dh.push_back(mine[i]->hp);
        for (int k = 0; k < producible; ++k) dh.push_back(whp);
        auto holds = [&]() { std::vector<int> d = dh; std::sort(d.begin(), d.end()); return !sim_siege(home_threat, d, hqhp0, hqtur0).win; };
        std::vector<int> rear, fieldforce;
        for (int i = 0; i < (int)mine.size(); ++i) {
          if (mine[i]->state != WState::STATIONARY || mine[i]->region == M.my_hq) continue;
          (owned(mine[i]->region) ? rear : fieldforce).push_back(i);
        }
        auto bydist = [&](int x, int y) { return P.dist[mine[x]->region][M.my_hq] < P.dist[mine[y]->region][M.my_hq]; };
        std::sort(rear.begin(), rear.end(), bydist);
        std::sort(fieldforce.begin(), fieldforce.end(), bydist);
        for (int i : rear) { if (holds()) break; if (omove(*mine[i], M.my_hq)) { committed[i] = 1; dh.push_back(mine[i]->hp); } }
        if (!holds()) {
          for (int i : fieldforce) { if (holds()) break; if (omove(*mine[i], M.my_hq)) { committed[i] = 1; dh.push_back(mine[i]->hp); } }
          G.emergency = true;
        }
        def_short = producible;
      } else {

        int have = 0;
        for (int i : army) {
          if (is_reserve[i] || mine[i]->state != WState::STATIONARY) continue;
          if (mine[i]->region == defense_node) { committed[i] = 1; if (++have >= def_need) break; }
        }

        std::vector<int> more;
        for (int i : army) { if (is_reserve[i] || mine[i]->state != WState::STATIONARY || committed[i]) continue;
          if (mine[i]->region != defense_node) more.push_back(i); }
        std::sort(more.begin(), more.end(), [&](int x, int y) { return P.dist[mine[x]->region][defense_node] < P.dist[mine[y]->region][defense_node]; });
        for (int i : more) { if (have >= def_need) break; if (omove(*mine[i], defense_node)) { committed[i] = 1; ++have; } }
        def_short = std::max(0, def_need - have);
      }
    }


    {

      if (rush) for (int i = 0; i < (int)mine.size(); ++i)
        if (!committed[i] && mine[i]->state == WState::STATIONARY && mine[i]->region != M.my_hq) omove(*mine[i], M.my_hq);


      if (!rush && defense_node < 0) {
        std::vector<int> avail;
        for (int i : army) if (!committed[i] && mine[i]->state == WState::STATIONARY && mine[i]->region == M.my_hq) avail.push_back(i);
        std::vector<const Building *> ebases;
        for (auto b : mybld) {
          if (b->region == M.my_hq || my_cnt(S, me, b->region) >= 1) continue;
          bool incoming = false; for (int j = 0; j < (int)mine.size(); ++j) if (mine[j]->state == WState::MOVING && mine[j]->target == b->region) incoming = true;
          if (!incoming) ebases.push_back(b);
        }
        std::sort(ebases.begin(), ebases.end(), [&](const Building *x, const Building *y) { return P.dist[M.my_hq][x->region] < P.dist[M.my_hq][y->region]; });
        size_t ai = 0;
        for (auto b : ebases) { if (ai >= avail.size()) break; int i = avail[ai++]; if (omove(*mine[i], b->region)) committed[i] = 1; }
        if (ebases.empty()) G.emergency = false;
      }


      if (!rush) {
        for (auto b : mybld) {
          if (b->type != BType::BASE) continue;
          int z = b->region;
          bool threatened = invasion;
          if (!threatened) {
            std::vector<char> near(M.N, 0); near[z] = 1; std::vector<int> fr = {z};
            for (int h = 0; h < 3 && !threatened; ++h) { std::vector<int> nx; for (int x : fr) for (int nb : M.adj[x]) if (!near[nb]) { near[nb] = 1; nx.push_back(nb); } fr.swap(nx); }
            for (const auto &w : S.warriors) { if (w.id.side == me || w.hp <= 0) continue; const Building *hb = bat(S, w.region); if (hb && hb->side != me) continue; if (near[w.region]) { threatened = true; break; } }
          }
          if (!threatened) continue;
          int want = invasion ? 2 : 1;
          int have = 0;
          for (int i : army) { if (committed[i] || mine[i]->state != WState::STATIONARY) continue; if (mine[i]->region == z) { committed[i] = 1; if (++have >= want) break; } }
          int incoming = 0; for (int j = 0; j < (int)mine.size(); ++j) if (mine[j]->state == WState::MOVING && mine[j]->target == z) ++incoming;
          while (have + incoming < want) {
            int best = -1; double bd = 1e18;
            for (int i : army) { if (committed[i] || mine[i]->state != WState::STATIONARY) continue; double d = P.dist[mine[i]->region][z]; if (d < bd) { bd = d; best = i; } }
            if (best < 0 || !omove(*mine[best], z)) break;
            committed[best] = 1; ++have;
          }
        }
      }


      bool center_recapture = false;
      if (!rush && !endgame && defense_node < 0 && is_sh(M, central)
          && my_side_region(M, P, central) && !(bat(S, central) && bat(S, central)->side == me)) {
        const Building *cb = bat(S, central);
        bool enemy_base = cb && cb->side != me;

        if (my_cnt(S, me, central) >= 1 && en_cnt(S, me, central) == 0 && cb == nullptr && budget >= BASE_LEVELS[1].cost) {
          bool dup = false; for (int u : a.upgrades) if (u == central) dup = true;
          if (!dup) { a.upgrades.push_back(central); budget -= BASE_LEVELS[1].cost; }
        }
        int atC = my_cnt(S, me, central); for (int j = 0; j < (int)mine.size(); ++j) if (mine[j]->state == WState::MOVING && mine[j]->target == central) ++atC;

        for (int i : army) if (!committed[i] && mine[i]->state == WState::STATIONARY && mine[i]->region == central) committed[i] = 1;
        int cap = (bases >= 1) ? 2 : 1;
        if (!enemy_base && G.center_sent < cap && atC < cap) {

          int best = -1; double bd = 1e18;
          for (int i : army) { if (committed[i] || mine[i]->state != WState::STATIONARY || mine[i]->region == central) continue;
            double d = P.dist[mine[i]->region][central]; if (d < bd) { bd = d; best = i; } }
          if (best >= 0 && omove(*mine[best], central)) { committed[best] = 1; ++G.center_sent; }
        } else if (G.center_sent >= 2 && atC < 1) {


          center_recapture = true;
        }
      }


      if (target >= 0 && defense_node < 0) for (int i : muster) if (!committed[i]) omove(*mine[i], target);


      if (!endgame && defense_node < 0) for (int i : army) {
        if (committed[i] || mine[i]->state != WState::STATIONARY) continue;
        int r = mine[i]->region;
        if (is_sh(M, r) && bat(S, r) == nullptr && en_cnt(S, me, r) == 0 && budget >= BASE_LEVELS[1].cost) {
          if (rush && r != frontyard) continue;
          bool dup = false; for (int u : a.upgrades) if (u == r) dup = true;
          if (!dup) { a.upgrades.push_back(r); budget -= BASE_LEVELS[1].cost; }
        }
      }


      std::vector<char> claimed(M.N, 0);
      for (int s : M.strongholds) if (owned(s)) claimed[s] = 1;
      for (int i = 0; i < (int)mine.size(); ++i) if (mine[i]->state == WState::MOVING && mine[i]->target >= 0 && mine[i]->target < M.N) claimed[mine[i]->target] = 1;
      std::vector<int> cand;
      if (rush) { if (frontyard >= 0) cand.push_back(frontyard); }
      else if (!endgame) {


        std::vector<int> rest; for (int s : M.strongholds) { if (s == central) continue; if (my_side_region(M, P, s) && !owned(s) && bat(S, s) == nullptr && en_cnt(S, me, s) == 0) rest.push_back(s); }
        std::sort(rest.begin(), rest.end(), [&](int x, int y) { return P.dist[M.my_hq][x] < P.dist[M.my_hq][y]; });
        for (int s : rest) cand.push_back(s);

        if (G.center_sent >= 2 && is_sh(M, central) && !owned(central) && bat(S, central) == nullptr && en_cnt(S, me, central) == 0) cand.push_back(central);
      }
      int settler_quota = rush ? 1 : (behind ? 1 : 2); int settlers = 0;
      std::vector<char> settled_here(M.N, 0);
      for (int i : army) {
        if (committed[i] || mine[i]->state != WState::STATIONARY) continue;
        if (is_reserve[i]) { int dst = (defense_node >= 0) ? defense_node : M.my_hq; if (mine[i]->region != dst) omove(*mine[i], dst); continue; }
        int r = mine[i]->region;
        bool can_settle_here = !endgame && defense_node < 0 && is_sh(M, r) && bat(S, r) == nullptr && en_cnt(S, me, r) == 0 && (!rush || r == frontyard);
        if (can_settle_here) {
          if (!settled_here[r]) { settled_here[r] = 1; claimed[r] = 1; continue; }
          if (r != rally_to) omove(*mine[i], rally_to);
          continue;
        }
        const Building *here = bat(S, r);
        if (here && here->side != me) continue;
        int dest = -1;
        if (!rush && !stop_expand && defense_node < 0 && settlers < settler_quota) for (int s : cand) if (!claimed[s]) { dest = s; break; }
        if (dest >= 0 && omove(*mine[i], dest)) { claimed[dest] = 1; ++settlers; continue; }
        if (r != rally_to) omove(*mine[i], rally_to);
      }


      if (def_short > 0) {
        int cap = HQ_LEVELS[hqlv].train_cap; int n = std::min({def_short, cap, budget / TRAIN_COST});
        if (n > 0) { a.train_n = n; budget -= TRAIN_COST * n; }
      } else if (rush || behind || invasion || center_recapture) {
        int cap = HQ_LEVELS[hqlv].train_cap; int n = std::min(cap, budget / TRAIN_COST);
        if (n > 0) { a.train_n = n; budget -= TRAIN_COST * n; }
      } else if (!need_up) {
        int need_cap = (obj_need >= 999) ? 6 : obj_need;
        int max_warriors = endgame ? 99999 : std::max(need_cap + 4, income_est / 2 + 2);
        int unowned_ourside = 0;
        if (is_sh(M, central) && !owned(central) && bat(S, central) == nullptr && en_cnt(S, me, central) == 0) ++unowned_ourside;
        for (int s : M.strongholds) if (s != central && my_side_region(M, P, s) && !owned(s) && bat(S, s) == nullptr && en_cnt(S, me, s) == 0) ++unowned_ourside;
        int reserve_gold = BASE_LEVELS[1].cost * std::min(1, unowned_ourside) + 2 * ntot + 30;


        int cap = HQ_LEVELS[hqlv].train_cap;
        int aff = std::max(0, budget - reserve_gold) / TRAIN_COST;
        int n = std::min({cap, aff, std::max(0, max_warriors - ntot)});
        if (n > 0) { a.train_n = n; budget -= TRAIN_COST * n; }
      }
    }


    if (want_hq_up) {
      int buffer = (behind_lv || myhq->level < 3) ? 60 : 250;
      bool dup = false; for (int u : a.upgrades) if (u == M.my_hq) dup = true;
      if (!dup && budget >= hq_cost + buffer) { a.upgrades.push_back(M.my_hq); budget -= hq_cost; }
    }
    (void)MAX_TURN; (void)UPKEEP_PER_WARRIOR;
    return a;
  }


  int main() {
    GameMap M; GameState S;
    parse_init(M, S);
    Paths P = calculate_paths(M);
    int turn;
    while (read_turn_start(turn)) {
      Actions a = decide(S, M, P, turn);
      emit_command(); emit_actions(a); emit_end();
      read_turn_result(S, M, a);
    }
    return 0;
  }
