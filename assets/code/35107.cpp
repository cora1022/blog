#include <bits/stdc++.h>
using namespace std;

using ll = long long;
static const ll INF = (1LL << 60);


static const int HQ_UPGRADE_COST[6] = {0, 0, 600, 1200, 2400, 3600};
static const int HQ_REPAIR_COST = 1000;
static const int HQ_WARRIOR_HP[6] = {0, 4, 5, 6, 7, 8};
static const int HQ_MAX_HP[6] = {0, 10, 15, 20, 25, 30};
static const int HQ_TURRET[6] = {0, 1, 2, 2, 3, 3};
static const int HQ_TRAIN_CAP[6] = {0, 1, 1, 2, 2, 3};
static const int HQ_LABOR_CAP[6] = {0, 1, 2, 3, 4, 5};

static const int BASE_UPGRADE_COST[4] = {0, 300, 600, 1000};
static const int BASE_REPAIR_COST = 500;
static const int BASE_MAX_HP[4] = {0, 6, 12, 18};
static const int BASE_TURRET[4] = {0, 1, 1, 2};
static const int BASE_LABOR_CAP[4] = {0, 1, 2, 3};

static const int MOVE_BASIC_COST = 10;
static const int TRAIN_COST = 120;
static const int BASE_BUILD_COST = 300;
static const int FOOD_COST = 2;
static const int LABOR_INCOME = 15;
static const ll GOLD_BUFFER = 25;


struct Warrior {
    char owner = '?';
    int zone = -1;
    int hp = 0;
    bool moving = false;
    int target = -1;
    bool alive = true;
};

struct Building {
    char owner = '?';
    int zone = -1;
    int level = 0;
    int hp = 0;
    bool isHQ = false;
};

struct ThreatInfo {
    bool hqEmergency = false;
    bool massHqRush = false;
    bool defenseMode = false;
    int defenseTarget = -1;
    int guardNeed = 0;
    int enemyAtHQ = 0;
    int enemyNearHQ = 0;
    int enemyInMyHalf = 0;
    int clusterZone = -1;
    int clusterSize = 0;
    int attackedBase = -1;
};


int N = 0, K = 0;
vector<ll> px, py;
vector<vector<int>> adj;
vector<char> isStronghold;
vector<int> strongholds;
vector<int> safeStrongholds;

vector<vector<ll>> distCache;
vector<char> distCached;
vector<ll> distMy, distOpp;
ll hqDistance = INF;

char meC = 'A', oppC = 'B';
int myHQ = 0, oppHQ = 0;
ll gold = 500;
int byoyomi = 5;

map<string, Warrior> warriors;
map<int, Building> buildings;

int currentAttackTarget = -1;
int rallyZone = -1;
bool hqPressure = false;
bool lastTurnMyHqSieged = false;
bool lastTurnOppHqSieged = false;
set<int> lastTurnMyBaseSieged;
set<int> pendingMyUpgrades;


bool isIntegerToken(const string& s) {
    if (s.empty()) return false;
    int i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == (int)s.size()) return false;
    for (; i < (int)s.size(); i++) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

char ownerFromToken(const string& s) {
    if (s == "LEFT") return 'A';
    if (s == "RIGHT") return 'B';
    return s.empty() ? '?' : s[0];
}

int hqZoneOf(char owner) {
    return owner == 'A' ? 0 : N - 1;
}

bool validZone(int z) {
    return 0 <= z && z < N;
}

bool distLePercent(ll a, ll b, int percent) {
    if (a >= INF / 2 || b >= INF / 2) return false;
    return (long double)a <= (long double)b * (long double)percent / 100.0L + 1e-9L;
}

bool distGtPercent(ll a, ll b, int percent) {
    if (a >= INF / 2 || b >= INF / 2) return false;
    return (long double)a > (long double)b * (long double)percent / 100.0L + 1e-9L;
}

bool isAliveMine(const Warrior& w) {
    return w.alive && w.owner == meC && validZone(w.zone);
}

bool isAliveOpp(const Warrior& w) {
    return w.alive && w.owner == oppC && validZone(w.zone);
}

bool hasMyHQ() {
    auto it = buildings.find(myHQ);
    return it != buildings.end() && it->second.owner == meC && it->second.isHQ;
}

int clampLevel(int level, bool isHQ) {
    if (isHQ) return max(1, min(5, level));
    return max(1, min(3, level));
}

int buildingMaxHp(const Building& b) {
    int level = clampLevel(b.level, b.isHQ);
    return b.isHQ ? HQ_MAX_HP[level] : BASE_MAX_HP[level];
}

int buildingTurret(const Building& b) {
    int level = clampLevel(b.level, b.isHQ);
    return b.isHQ ? HQ_TURRET[level] : BASE_TURRET[level];
}

int buildingLaborCap(const Building& b) {
    int level = clampLevel(b.level, b.isHQ);
    return b.isHQ ? HQ_LABOR_CAP[level] : BASE_LABOR_CAP[level];
}

int hqLevelOf(char owner) {
    int z = hqZoneOf(owner);
    auto it = buildings.find(z);
    if (it == buildings.end() || !it->second.isHQ) return 1;
    return clampLevel(it->second.level, true);
}

int hqHpOf(char owner) {
    int z = hqZoneOf(owner);
    auto it = buildings.find(z);
    if (it == buildings.end() || !it->second.isHQ) return 0;
    return it->second.hp;
}

int countWarriors(char owner) {
    int cnt = 0;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (w.alive && w.owner == owner) cnt++;
    }
    return cnt;
}

int countAt(char owner, int zone, bool idleOnly = false) {
    int cnt = 0;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (!w.alive || w.owner != owner || w.zone != zone) continue;
        if (idleOnly && w.moving) continue;
        cnt++;
    }
    return cnt;
}

int myAt(int zone) {
    return countAt(meC, zone, false);
}

int oppAt(int zone) {
    return countAt(oppC, zone, false);
}

int myIdleAt(int zone) {
    return countAt(meC, zone, true);
}

int movingTo(char owner, int zone) {
    int cnt = 0;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (w.alive && w.owner == owner && w.moving && w.target == zone) cnt++;
    }
    return cnt;
}

int plannedMyStaff(int zone) {
    int cnt = 0;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (!isAliveMine(w)) continue;
        if (w.zone == zone || (w.moving && w.target == zone)) cnt++;
    }
    return cnt;
}

bool myBuildingAt(int zone) {
    auto it = buildings.find(zone);
    return it != buildings.end() && it->second.owner == meC;
}

bool oppBuildingAt(int zone) {
    auto it = buildings.find(zone);
    return it != buildings.end() && it->second.owner == oppC;
}

int moveCostTo(int dest) {
    return myBuildingAt(dest) ? 0 : MOVE_BASIC_COST;
}

int myBaseCount() {
    int cnt = 0;
    for (const auto& kv : buildings) {
        if (kv.second.owner == meC && !kv.second.isHQ) cnt++;
    }
    return cnt;
}

int oppBaseCount() {
    int cnt = 0;
    for (const auto& kv : buildings) {
        if (kv.second.owner == oppC && !kv.second.isHQ) cnt++;
    }
    return cnt;
}


ll edgeLength(int a, int b) {
    long double dx = (long double)px[a] - (long double)px[b];
    long double dy = (long double)py[a] - (long double)py[b];
    return (ll)ceill(sqrtl(dx * dx + dy * dy) - 1e-12L);
}

vector<ll> runDijkstra(int src) {
    vector<ll> dist(N, INF);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> pq;
    dist[src] = 0;
    pq.push({0, src});
    while (!pq.empty()) {
        auto [du, u] = pq.top();
        pq.pop();
        if (du != dist[u]) continue;
        for (int v : adj[u]) {
            if (!validZone(v)) continue;
            ll nd = du + edgeLength(u, v);
            if (nd < dist[v]) {
                dist[v] = nd;
                pq.push({nd, v});
            }
        }
    }
    return dist;
}

const vector<ll>& distFrom(int src) {
    static vector<ll> bad;
    if (!validZone(src)) return bad;
    if (!distCached[src]) {
        distCache[src] = runDijkstra(src);
        distCached[src] = 1;
    }
    return distCache[src];
}

ll mapDist(int a, int b) {
    if (!validZone(a) || !validZone(b)) return INF;
    const vector<ll>& d = distFrom(a);
    if ((int)d.size() != N) return INF;
    return d[b];
}


void parseReady() {
    string side;
    cin >> side;
    meC = (side == "LEFT") ? 'A' : 'B';
    oppC = (meC == 'A') ? 'B' : 'A';

    string token;
    cin >> token;
    if (token == "MAP") {
        cin >> N >> K;
    } else {
        N = stoi(token);
        cin >> K;
    }

    px.assign(N, 0);
    py.assign(N, 0);
    for (int i = 0; i < N; i++) cin >> px[i];
    for (int i = 0; i < N; i++) cin >> py[i];

    isStronghold.assign(N, 0);
    strongholds.clear();
    string firstStronghold;
    cin >> firstStronghold;
    if (firstStronghold == "STRONGHOLDS") {
        for (int i = 0; i < K; i++) {
            int z;
            cin >> z;
            if (validZone(z)) {
                isStronghold[z] = 1;
                strongholds.push_back(z);
            }
        }
    } else {
        int z = stoi(firstStronghold);
        if (validZone(z)) {
            isStronghold[z] = 1;
            strongholds.push_back(z);
        }
        for (int i = 1; i < K; i++) {
            cin >> z;
            if (validZone(z)) {
                isStronghold[z] = 1;
                strongholds.push_back(z);
            }
        }
    }

    adj.assign(N, {});
    for (int i = 0; i < N; i++) {
        int deg;
        cin >> deg;
        adj[i].resize(deg);
        for (int j = 0; j < deg; j++) cin >> adj[i][j];
    }

    myHQ = hqZoneOf(meC);
    oppHQ = hqZoneOf(oppC);
    gold = 500;
    byoyomi = 5;
    hqPressure = false;
    lastTurnMyHqSieged = false;
    lastTurnOppHqSieged = false;
    lastTurnMyBaseSieged.clear();
    pendingMyUpgrades.clear();

    buildings.clear();
    warriors.clear();
    buildings[0] = {'A', 0, 1, HQ_MAX_HP[1], true};
    buildings[N - 1] = {'B', N - 1, 1, HQ_MAX_HP[1], true};
    for (int i = 1; i <= 3; i++) {
        warriors[string("A") + to_string(i)] = {'A', 0, HQ_WARRIOR_HP[1], false, -1, true};
        warriors[string("B") + to_string(i)] = {'B', N - 1, HQ_WARRIOR_HP[1], false, -1, true};
    }

    distCache.assign(N, {});
    distCached.assign(N, 0);
    distMy = distFrom(myHQ);
    distOpp = distFrom(oppHQ);
    hqDistance = validZone(oppHQ) ? distMy[oppHQ] : INF;

    safeStrongholds = strongholds;
    sort(safeStrongholds.begin(), safeStrongholds.end(), [&](int a, int b) {
        bool aSafe = distMy[a] <= distOpp[a];
        bool bSafe = distMy[b] <= distOpp[b];
        if (aSafe != bSafe) return aSafe > bSafe;
        if (distMy[a] != distMy[b]) return distMy[a] < distMy[b];
        ll ca = llabs(distMy[a] - distOpp[a]);
        ll cb = llabs(distMy[b] - distOpp[b]);
        if (ca != cb) return ca < cb;
        return a < b;
    });

    cout << "OK\n" << flush;
}


void applyUpgradeResult(char owner, int zone) {
    if (!validZone(zone)) return;
    bool isHQ = (zone == hqZoneOf(owner));
    auto it = buildings.find(zone);
    if (it == buildings.end()) {
        if (isHQ) {
            buildings[zone] = {owner, zone, 1, HQ_MAX_HP[1], true};
        } else {
            buildings[zone] = {owner, zone, 1, BASE_MAX_HP[1], false};
        }
        return;
    }

    Building& b = it->second;
    if (b.owner != owner) {
        b.owner = owner;
        b.isHQ = isHQ;
        b.level = 1;
        b.hp = isHQ ? HQ_MAX_HP[1] : BASE_MAX_HP[1];
        return;
    }

    int maxLevel = b.isHQ ? 5 : 3;
    if (b.level < maxLevel) b.level++;
    b.hp = buildingMaxHp(b);
}

void removeDeadUnitsAndBuildings() {
    for (auto it = warriors.begin(); it != warriors.end();) {
        if (!it->second.alive || it->second.hp <= 0) {
            it = warriors.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = buildings.begin(); it != buildings.end();) {
        if (it->second.hp <= 0) {
            it = buildings.erase(it);
        } else {
            ++it;
        }
    }
}

void parseTimeLine() {
    string a;
    cin >> a;
    if (a == "LEFT" || a == "RIGHT") {
        string side1 = a, side2;
        ll used1 = 0, rem1 = 0, used2 = 0, rem2 = 0;
        cin >> used1 >> rem1 >> side2 >> used2 >> rem2;
        if ((meC == 'A' && side1 == "LEFT") || (meC == 'B' && side1 == "RIGHT")) {
            byoyomi = (int)rem1;
        } else {
            byoyomi = (int)rem2;
        }
    } else {
        ll tx = stoll(a), rx = 0, ty = 0, ry = 0;
        cin >> rx >> ty >> ry;


        byoyomi = (int)rx;
        (void)tx;
        (void)ty;
        (void)ry;
    }
}

void parseResultBody(string firstSection) {
    vector<pair<char, int>> upgrades;
    vector<string> trains;
    vector<pair<string, int>> moves;
    vector<tuple<string, string, int>> damages;
    vector<tuple<char, int, int>> sieges;

    string sec = firstSection;
    while (true) {
        if (sec.empty() && !(cin >> sec)) break;
        if (sec == "END") {
            string rest;
            getline(cin, rest);
            break;
        }

        if (sec == "TIME") {
            parseTimeLine();
        } else if (sec == "UPGRADE") {
            string x;
            cin >> x;
            if (isIntegerToken(x)) {
                int n = stoi(x);
                for (int i = 0; i < n; i++) {
                    string ownerToken;
                    int zone;
                    cin >> ownerToken >> zone;
                    upgrades.push_back({ownerFromToken(ownerToken), zone});
                }
            } else {
                int zone;
                cin >> zone;
                upgrades.push_back({ownerFromToken(x), zone});
                string rest;
                getline(cin, rest);
                stringstream ss(rest);
                string ownerToken;
                while (ss >> ownerToken >> zone) {
                    upgrades.push_back({ownerFromToken(ownerToken), zone});
                }
            }
        } else if (sec == "TRAIN") {
            string x;
            cin >> x;
            if (isIntegerToken(x)) {
                int n = stoi(x);
                for (int i = 0; i < n; i++) {
                    string id;
                    cin >> id;
                    trains.push_back(id);
                }
            } else {
                trains.push_back(x);
                string rest;
                getline(cin, rest);
                stringstream ss(rest);
                string id;
                while (ss >> id) trains.push_back(id);
            }
        } else if (sec == "MOVE") {
            string x;
            cin >> x;
            if (isIntegerToken(x)) {
                int n = stoi(x);
                for (int i = 0; i < n; i++) {
                    string id;
                    int zone;
                    cin >> id >> zone;
                    moves.push_back({id, zone});
                }
            } else {
                int zone;
                cin >> zone;
                moves.push_back({x, zone});
                string rest;
                getline(cin, rest);
                stringstream ss(rest);
                string id;
                while (ss >> id >> zone) {
                    moves.push_back({id, zone});
                }
            }
        } else if (sec == "DAMAGE") {
            string x;
            cin >> x;
            if (isIntegerToken(x)) {
                int n = stoi(x);
                for (int i = 0; i < n; i++) {
                    string kind, id;
                    int hp;
                    cin >> kind >> id >> hp;
                    damages.push_back({kind, id, hp});
                }
            } else {
                string id;
                int hp;
                cin >> id >> hp;
                damages.push_back({x, id, hp});
                string rest;
                getline(cin, rest);
                stringstream ss(rest);
                string kind;
                while (ss >> kind >> id >> hp) {
                    damages.push_back({kind, id, hp});
                }
            }
        } else if (sec == "SIEGE") {
            string x;
            cin >> x;
            if (isIntegerToken(x)) {
                int n = stoi(x);
                for (int i = 0; i < n; i++) {
                    string ownerToken;
                    int zone, hp;
                    cin >> ownerToken >> zone >> hp;
                    sieges.push_back({ownerFromToken(ownerToken), zone, hp});
                }
            } else {
                int zone, hp;
                cin >> zone >> hp;
                sieges.push_back({ownerFromToken(x), zone, hp});
                string rest;
                getline(cin, rest);
                stringstream ss(rest);
                string ownerToken;
                while (ss >> ownerToken >> zone >> hp) {
                    sieges.push_back({ownerFromToken(ownerToken), zone, hp});
                }
            }
        } else {
            string rest;
            getline(cin, rest);
        }

        sec.clear();
    }

    lastTurnMyHqSieged = false;
    lastTurnOppHqSieged = false;
    lastTurnMyBaseSieged.clear();


    for (auto [owner, zone] : upgrades) {
        if (owner == meC && pendingMyUpgrades.count(zone)) continue;
        applyUpgradeResult(owner, zone);
    }


    for (const string& id : trains) {
        if (id.empty()) continue;
        char owner = id[0];
        int level = hqLevelOf(owner);
        warriors[id] = {owner, hqZoneOf(owner), HQ_WARRIOR_HP[level], false, -1, true};
    }


    for (auto [id, zone] : moves) {
        auto it = warriors.find(id);
        if (it == warriors.end()) continue;
        Warrior& w = it->second;
        w.zone = zone;
        if (w.owner == meC && w.moving && w.zone == w.target) {
            w.moving = false;
            w.target = -1;
        }
    }


    for (auto [kind, id, hp] : damages) {
        if (kind == "HUNGER") continue;
        auto it = warriors.find(id);
        if (it != warriors.end()) it->second.hp -= hp;
    }


    for (auto [owner, zone, hp] : sieges) {
        auto it = buildings.find(zone);
        if (it != buildings.end()) it->second.hp -= hp;
        if (owner == meC && zone == myHQ) lastTurnMyHqSieged = true;
        if (owner == meC && zone != myHQ) lastTurnMyBaseSieged.insert(zone);
        if (owner == oppC && zone == oppHQ) {
            lastTurnOppHqSieged = true;
            hqPressure = true;
        }
    }

    removeDeadUnitsAndBuildings();


    ll income = 0;
    for (const auto& kv : buildings) {
        const Building& b = kv.second;
        if (b.owner != meC) continue;
        income += 1LL * LABOR_INCOME * min(myAt(kv.first), buildingLaborCap(b));
    }
    gold += income;


    gold -= 1LL * FOOD_COST * countWarriors(meC);
    if (gold < 0) gold = 0;


    for (auto [kind, id, hp] : damages) {
        if (kind != "HUNGER") continue;
        auto it = warriors.find(id);
        if (it != warriors.end()) it->second.hp -= hp;
    }
    removeDeadUnitsAndBuildings();
    pendingMyUpgrades.clear();
}

void parseTurnResult() {
    int turnNumber = 0;
    cin >> turnNumber;
    string first;
    if (!(cin >> first)) return;
    if (first == "RESULT") {
        if (!(cin >> first)) return;
    }
    parseResultBody(first);
}


ThreatInfo analyzeThreat(int day) {
    ThreatInfo th;
    th.defenseTarget = myHQ;
    vector<int> enemyByZone(N, 0);

    ll nearLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 35 / 100);
    ll rushLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 45 / 100);
    ll lateRushLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 55 / 100);
    ll hugeRushLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 65 / 100);
    ll overwhelmingRushLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 70 / 100);
    ll middleLimit = (hqDistance >= INF / 2) ? INF / 2 : max(1LL, hqDistance * 55 / 100);
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (!isAliveOpp(w)) continue;
        enemyByZone[w.zone]++;
        if (w.zone == myHQ) th.enemyAtHQ++;
        if (distMy[w.zone] <= nearLimit) th.enemyNearHQ++;
        if (distMy[w.zone] <= distOpp[w.zone] || distMy[w.zone] <= middleLimit) {
            th.enemyInMyHalf++;
        }
    }

    for (int z = 0; z < N; z++) {
        if (enemyByZone[z] > th.clusterSize ||
            (enemyByZone[z] == th.clusterSize && th.clusterZone >= 0 && distMy[z] < distMy[th.clusterZone])) {
            th.clusterSize = enemyByZone[z];
            th.clusterZone = z;
        }
    }

    ll bestAttackedDist = INF;
    for (const auto& kv : buildings) {
        int z = kv.first;
        const Building& b = kv.second;
        if (b.owner != meC || b.isHQ) continue;
        bool attacked = oppAt(z) > 0 || lastTurnMyBaseSieged.count(z);
        if (!attacked) continue;
        if (distMy[z] < bestAttackedDist) {
            bestAttackedDist = distMy[z];
            th.attackedBase = z;
        }
    }

    int myHqHp = hqHpOf(meC);
    bool hqLow = myHqHp > 0 && myHqHp <= max(5, HQ_MAX_HP[hqLevelOf(meC)] / 2);
    if (th.clusterZone >= 0) {
        bool bigNearRush = th.clusterSize >= 5 && distMy[th.clusterZone] <= rushLimit;
        bool hugeLateRush = th.clusterSize >= 8 && distMy[th.clusterZone] <= lateRushLimit;
        bool hugeMidRush = th.clusterSize >= 10 && distMy[th.clusterZone] <= hugeRushLimit;
        bool overwhelmingRush = day >= 60 && th.clusterSize >= 12 &&
                                distMy[th.clusterZone] <= overwhelmingRushLimit;
        bool lateAnyRush = day >= 150 && th.clusterSize >= 5 && distMy[th.clusterZone] <= lateRushLimit;
        th.massHqRush = bigNearRush || hugeLateRush || hugeMidRush || overwhelmingRush || lateAnyRush;
    }
    th.hqEmergency = th.enemyAtHQ > 0 || lastTurnMyHqSieged ||
                     th.enemyNearHQ >= 3 || th.massHqRush || (hqLow && th.enemyNearHQ > 0);
    if (day >= 170 && hqLow && th.enemyInMyHalf > 0) th.hqEmergency = true;

    th.defenseMode = th.hqEmergency || th.attackedBase >= 0 || th.enemyInMyHalf >= 3;

    if (th.hqEmergency) {
        th.defenseTarget = myHQ;
    } else if (th.attackedBase >= 0) {
        th.defenseTarget = th.attackedBase;
    } else if (th.clusterZone >= 0 && th.enemyInMyHalf >= 3) {
        int best = myHQ;
        ll bestD = mapDist(myHQ, th.clusterZone);
        for (const auto& kv : buildings) {
            int z = kv.first;
            const Building& b = kv.second;
            if (b.owner != meC) continue;
            if (oppAt(z) > 0) continue;
            ll d = mapDist(z, th.clusterZone);
            if (d < bestD) {
                bestD = d;
                best = z;
            }
        }
        th.defenseTarget = best;
    }

    int turret = 0;
    auto bit = buildings.find(th.defenseTarget);
    if (bit != buildings.end()) turret = buildingTurret(bit->second);

    if (th.hqEmergency) {
        th.guardNeed = max({4, th.enemyAtHQ + 4, th.enemyNearHQ + 2, th.clusterSize + 1 - turret});
        if (th.massHqRush) th.guardNeed = max(th.guardNeed, th.clusterSize + 4);
        if (lastTurnMyHqSieged) th.guardNeed += 2;
    } else if (th.attackedBase >= 0) {
        th.guardNeed = max(3, oppAt(th.attackedBase) + 3 - turret);
    } else if (th.enemyInMyHalf >= 3) {
        th.guardNeed = max(3, th.clusterSize + 2 - turret);
    }
    th.guardNeed = max(0, th.guardNeed);
    return th;
}

bool isGatewayBase(int z) {
    auto it = buildings.find(z);
    if (it == buildings.end()) return false;
    const Building& b = it->second;
    if (b.owner != oppC || b.isHQ) return false;
    if (distMy[z] >= INF / 2 || distOpp[z] >= INF / 2 || hqDistance >= INF / 2) return false;
    if (distMy[z] <= distOpp[z]) return false;
    if (!distLePercent(distOpp[z], hqDistance, 45)) return false;
    if (!distLePercent(distMy[z], hqDistance, 82)) return false;
    if (!distLePercent(distMy[z] + distOpp[z], hqDistance, 112)) return false;
    return true;
}

bool isInnerRingBase(int z) {
    auto it = buildings.find(z);
    if (it == buildings.end()) return false;
    const Building& b = it->second;
    if (b.owner != oppC || b.isHQ) return false;
    if (distMy[z] >= INF / 2 || distOpp[z] >= INF / 2 || hqDistance >= INF / 2) return false;
    if (!distLePercent(distOpp[z], hqDistance, 25)) return false;
    if (!distLePercent(distMy[z], hqDistance, 96)) return false;
    if (!distLePercent(distMy[z] + distOpp[z], hqDistance, 112)) return false;
    return true;
}

int chooseGatewayTarget() {
    int best = -1;
    long double bestScore = 1e100L;
    for (const auto& kv : buildings) {
        int z = kv.first;
        if (!isGatewayBase(z)) continue;

        long double score = (long double)distMy[z] * 1.15L + (long double)distOpp[z] * 0.35L;
        score += (long double)oppAt(z) * 2500.0L;
        score += (long double)buildingTurret(kv.second) * 800.0L;
        if (z == currentAttackTarget) score -= 1200.0L;
        if (score < bestScore) {
            bestScore = score;
            best = z;
        }
    }
    return best;
}

int chooseInnerRingTarget(int day) {
    if (day >= 170) return -1;

    int best = -1;
    long double bestScore = 1e100L;
    for (const auto& kv : buildings) {
        int z = kv.first;
        if (!isInnerRingBase(z)) continue;

        const Building& b = kv.second;
        long double score = (long double)distMy[z] * 0.65L + (long double)distOpp[z] * 1.15L;
        score += (long double)oppAt(z) * 2600.0L;
        score += (long double)buildingTurret(b) * 900.0L;
        score -= (long double)(buildingMaxHp(b) - b.hp) * 180.0L;
        if (z == currentAttackTarget) score -= 1800.0L;
        if (score < bestScore) {
            bestScore = score;
            best = z;
        }
    }
    return best;
}

int choosePressureTarget(int day) {
    int gateway = chooseGatewayTarget();
    if (gateway >= 0) return gateway;
    int ring = chooseInnerRingTarget(day);
    if (ring >= 0) return ring;
    return oppHQ;
}

int chooseAttackTarget(const ThreatInfo& th, int day) {
    if (myAt(oppHQ) > 0 || lastTurnOppHqSieged) return oppHQ;
    if (hqPressure) {
        return choosePressureTarget(day);
    }

    int myArmy = countWarriors(meC);
    int oppArmy = countWarriors(oppC);
    int myForwardArmy = 0;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (!isAliveMine(w)) continue;
        bool inFront = (hqDistance < INF / 2 && distLePercent(hqDistance, distMy[w.zone] * 2, 100)) ||
                       distMy[w.zone] >= distOpp[w.zone];
        if (inFront) myForwardArmy++;
    }


    if (oppBaseCount() == 0 && countWarriors(meC) >= 7) {
        hqPressure = true;
        return choosePressureTarget(day);
    }
    if (!th.hqEmergency && day >= 125 && myArmy >= 18 &&
        myForwardArmy >= max(7, myArmy / 3) &&
        (myArmy >= oppArmy - 4 || myBaseCount() >= oppBaseCount() || myBaseCount() >= 7)) {
        hqPressure = true;
        return choosePressureTarget(day);
    }
    if (!th.hqEmergency && day >= 130 && myArmy >= 18 &&
        myBaseCount() >= 6 && myArmy + 10 >= oppArmy) {
        hqPressure = true;
        return choosePressureTarget(day);
    }
    if (!th.hqEmergency && day >= 150 && myArmy >= 16 &&
        (myArmy >= oppArmy - 8 || myForwardArmy >= 12) && myBaseCount() >= 5) {
        hqPressure = true;
        return choosePressureTarget(day);
    }
    if (!th.hqEmergency && day >= 175 && myArmy >= 10 && myArmy + 5 >= oppArmy) {
        hqPressure = true;
        return choosePressureTarget(day);
    }
    if (day >= 165 && countWarriors(meC) >= max(8, countWarriors(oppC) + 4) &&
        (hqHpOf(meC) >= hqHpOf(oppC) || day >= 190 ||
         countWarriors(meC) >= countWarriors(oppC) + 12)) {
        hqPressure = true;
        return choosePressureTarget(day);
    }

    int bestForwardBase = -1;
    long double bestForwardScore = 1e100L;
    for (const auto& kv : buildings) {
        int z = kv.first;
        const Building& b = kv.second;
        if (b.owner != oppC || b.isHQ) continue;
        if (distOpp[z] >= INF / 2 || distMy[z] >= INF / 2) continue;


        bool forwardThreat = distLePercent(distMy[z], hqDistance, 70) ||
                             distLePercent(distMy[z], distOpp[z], 130);
        if (!forwardThreat) continue;

        long double score = (long double)distMy[z] * 1.20L - (long double)distOpp[z] * 0.20L;
        if (distMy[z] <= distOpp[z]) score -= (long double)hqDistance * 0.08L;
        score += (long double)oppAt(z) * 3000.0L;
        if (score < bestForwardScore) {
            bestForwardScore = score;
            bestForwardBase = z;
        }
    }
    if (bestForwardBase >= 0) return bestForwardBase;

    if (th.enemyInMyHalf >= 3 && th.clusterZone >= 0) return th.clusterZone;

    int nearestBase = -1;
    ll nearestD = INF;
    for (const auto& kv : buildings) {
        int z = kv.first;
        const Building& b = kv.second;
        if (b.owner != oppC || b.isHQ) continue;
        if (distMy[z] < nearestD) {
            nearestD = distMy[z];
            nearestBase = z;
        }
    }
    if (nearestBase >= 0) return nearestBase;
    return oppHQ;
}

int chooseRallyZone(int target) {

    int best = myHQ;
    long double bestScore = 1e100L;
    for (const auto& kv : buildings) {
        int z = kv.first;
        const Building& b = kv.second;
        if (b.owner != meC) continue;
        if (oppAt(z) > 0) continue;
        ll d = mapDist(z, target);
        if (d >= INF / 2) continue;

        long double score = (long double)d;
        if (b.isHQ) score += (long double)hqDistance * 0.12L;
        else score -= (long double)distMy[z] * 0.08L;
        if (!b.isHQ && distGtPercent(distMy[z], distOpp[z], 130)) {
            score += (long double)hqDistance * 0.15L;
        }
        if (b.hp * 3 < buildingMaxHp(b)) score += (long double)hqDistance * 0.10L;
        if (z == rallyZone) score -= 50.0L;

        if (score < bestScore) {
            bestScore = score;
            best = z;
        }
    }
    return best;
}

vector<string> idleIdsAt(int zone) {
    vector<string> ids;
    for (const auto& kv : warriors) {
        const Warrior& w = kv.second;
        if (isAliveMine(w) && !w.moving && w.zone == zone) ids.push_back(kv.first);
    }
    return ids;
}

set<string> reserveWorkers(const ThreatInfo& th) {
    set<string> reserved;
    for (const auto& kv : buildings) {
        int z = kv.first;
        const Building& b = kv.second;
        if (b.owner != meC) continue;
        vector<string> ids = idleIdsAt(z);
        int need = buildingLaborCap(b);
        if (th.hqEmergency) {
            need = (z == myHQ) ? min(1, need) : 0;
        } else if (th.defenseMode) {
            need = (z == th.defenseTarget) ? min(need, 1) : min(need, 1);
        } else if (z == rallyZone) {
            need = min(need, max(1, buildingLaborCap(b)));
        }
        need = min(need, (int)ids.size());
        for (int i = 0; i < need; i++) reserved.insert(ids[i]);
    }
    return reserved;
}

vector<string> freeWarriors(const set<string>& reserved, bool releaseWorkers) {
    vector<string> ids;
    for (const auto& kv : warriors) {
        const string& id = kv.first;
        const Warrior& w = kv.second;
        if (!isAliveMine(w) || w.moving) continue;
        if (!releaseWorkers && reserved.count(id)) continue;
        ids.push_back(id);
    }
    return ids;
}

int attackRequirement(int target, int day) {
    int turret = 0;
    auto it = buildings.find(target);
    if (it != buildings.end()) turret = buildingTurret(it->second);

    int req = 4;
    if (target == oppHQ) {
        req = max(9, oppAt(target) + turret + 7);
        if (hqHpOf(oppC) > 0 && hqHpOf(oppC) <= 6) req -= 2;
        if (day >= 175) req -= 2;
        if (oppBaseCount() <= 1 && countWarriors(meC) >= countWarriors(oppC) + 2) req -= 1;
        return max(6, req);
    }

    if (it != buildings.end() && it->second.owner == oppC) {
        req = max(4, oppAt(target) + turret + 2);
        bool hardBase = isGatewayBase(target) || isInnerRingBase(target);
        if (hardBase) req = max(req, oppAt(target) + turret + 7);
        if (myAt(target) + movingTo(meC, target) > 0) req -= 1;
        if (day >= 160) req -= 1;
        return max(hardBase ? 8 : 3, req);
    }

    req = max(3, oppAt(target) + 1);
    return req;
}

bool shouldLaunchAttack(int target, int rally, int day, const vector<string>& freeIds) {
    int req = attackRequirement(target, day);
    int committed = myAt(target) + movingTo(meC, target);
    int atRally = myIdleAt(rally);
    int mobileForce = committed + atRally + (int)freeIds.size();
    int nearForce = committed + atRally;
    ll rallyToTarget = mapDist(rally, target);
    for (const string& id : freeIds) {
        auto it = warriors.find(id);
        if (it == warriors.end()) continue;
        if (it->second.zone == rally) continue;
        ll d = mapDist(it->second.zone, target);
        if (d < INF / 2 && (rallyToTarget >= INF / 2 || d <= rallyToTarget)) nearForce++;
    }

    if (target == oppHQ) {
        if (committed > 0 && committed + atRally >= max(4, oppAt(target) + 2)) return true;
        if (committed > 0 && (hqHpOf(oppC) <= 6 || day >= 195)) return true;
        if (day >= 190 && committed + atRally >= max(6, req - 3)) return true;
        if (day >= 185 && mobileForce >= req + 10 && atRally >= max(6, req / 2)) return true;
        return committed + atRally >= req;
    }

    if (isGatewayBase(target) || isInnerRingBase(target)) {
        if (committed >= req) return true;
        if (nearForce >= req) return true;
        if (committed > 0 && committed + atRally >= max(6, oppAt(target) + 3)) return true;
        if (committed > 0 && nearForce >= max(6, oppAt(target) + 3)) return true;
        if (day >= 185 && mobileForce >= req + 6 && atRally >= max(5, req / 2)) return true;
        return committed + atRally >= req;
    }

    if (hqPressure) return committed + atRally >= req || mobileForce >= req + 1;
    if (committed > 0) return true;
    if (day >= 170 && committed + (int)freeIds.size() >= max(3, req - 2)) return true;
    return committed + atRally >= req;
}

bool safeExpansionZone(int z, int earlyGoal, int desiredGoal) {
    auto expansionAllowed = [&](int zone) {
        if (!validZone(zone) || !isStronghold[zone]) return false;
        if (buildings.count(zone)) return false;
        if (oppAt(zone) > 0) return false;
        if (distMy[zone] >= INF / 2 || distOpp[zone] >= INF / 2) return false;
        if (myBaseCount() < earlyGoal) {
            bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[zone], hqDistance, 75);
            return distLePercent(distMy[zone], distOpp[zone], 130) && nearEnough;
        }
        if (myBaseCount() < desiredGoal) {
            bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[zone], hqDistance, 62);
            return distLePercent(distMy[zone], distOpp[zone], 115) && nearEnough;
        }
        bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[zone], hqDistance, 60);
        return distLePercent(distMy[zone], distOpp[zone], 105) && nearEnough;
    };

    if (!expansionAllowed(z)) return false;
    if (plannedMyStaff(z) > 0) return false;
    return true;
}

bool expansionBuildSiteAllowed(int z, int earlyGoal, int desiredGoal) {
    if (!validZone(z) || !isStronghold[z]) return false;
    if (buildings.count(z)) return false;
    if (oppAt(z) > 0) return false;
    if (distMy[z] >= INF / 2 || distOpp[z] >= INF / 2) return false;
    if (myBaseCount() < earlyGoal) {
        bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[z], hqDistance, 75);
        return distLePercent(distMy[z], distOpp[z], 130) && nearEnough;
    }
    if (myBaseCount() < desiredGoal) {
        bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[z], hqDistance, 62);
        return distLePercent(distMy[z], distOpp[z], 115) && nearEnough;
    }
    bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[z], hqDistance, 60);
    return distLePercent(distMy[z], distOpp[z], 105) && nearEnough;
}


void doTurn(int day) {
    vector<string> commands;
    set<int> upgradedToday;
    bool trainedToday = false;
    ll availableGold = max(0LL, gold - GOLD_BUFFER);
    ll protectedMoveReserve = 0;
    pendingMyUpgrades.clear();

    if (!hasMyHQ()) {
        cout << "COMMAND\nEND\n" << flush;
        return;
    }
    int commandStartHQLevel = hqLevelOf(meC);

    auto addCommand = [&](const string& s) {
        commands.push_back(s);
    };


    auto safeMove = [&](const string& id, int dest) -> bool {
        if (!validZone(dest)) return false;
        auto it = warriors.find(id);
        if (it == warriors.end()) return false;
        Warrior& w = it->second;
        if (!isAliveMine(w)) return false;
        if (w.moving) return false;
        if (w.zone == dest) return false;
        int cost = moveCostTo(dest);
        if (cost > 0 && availableGold - protectedMoveReserve < cost) return false;
        availableGold -= cost;
        gold -= cost;
        w.moving = true;
        w.target = dest;
        addCommand("MOVE " + id + " " + to_string(dest));
        return true;
    };

    auto safeUpgrade = [&](int zone) -> bool {
        if (!validZone(zone)) return false;
        if (upgradedToday.count(zone)) return false;
        if (myIdleAt(zone) <= 0) return false;
        if (oppAt(zone) > 0) return false;

        ll cost = 0;
        Building nextState;
        bool hasNextState = false;
        auto it = buildings.find(zone);
        if (it == buildings.end()) {
            if (!isStronghold[zone]) return false;
            cost = BASE_BUILD_COST;
            nextState = {meC, zone, 1, BASE_MAX_HP[1], false};
            hasNextState = true;
        } else {
            Building b = it->second;
            if (b.owner != meC) return false;
            if (b.isHQ) {
                if (b.level < 5) {
                    cost = HQ_UPGRADE_COST[b.level + 1];
                    b.level++;
                    b.hp = HQ_MAX_HP[b.level];
                } else {
                    if (b.hp >= HQ_MAX_HP[5]) return false;
                    cost = HQ_REPAIR_COST;
                    b.hp = HQ_MAX_HP[5];
                }
            } else {
                if (b.level < 3) {
                    cost = BASE_UPGRADE_COST[b.level + 1];
                    b.level++;
                    b.hp = BASE_MAX_HP[b.level];
                } else {
                    if (b.hp >= BASE_MAX_HP[3]) return false;
                    cost = BASE_REPAIR_COST;
                    b.hp = BASE_MAX_HP[3];
                }
            }
            nextState = b;
            hasNextState = true;
        }

        if (!hasNextState || availableGold < cost) return false;
        availableGold -= cost;
        gold -= cost;
        buildings[zone] = nextState;
        upgradedToday.insert(zone);
        pendingMyUpgrades.insert(zone);
        addCommand("UPGRADE " + to_string(zone));
        return true;
    };

    auto safeTrain = [&](int count) -> bool {
        if (trainedToday || count <= 0 || !hasMyHQ()) return false;
        int level = commandStartHQLevel;
        int cap = HQ_TRAIN_CAP[level];
        if (count > cap) return false;
        ll cost = 1LL * TRAIN_COST * count;
        if (availableGold < cost) return false;
        availableGold -= cost;
        gold -= cost;
        trainedToday = true;
        addCommand("TRAIN " + to_string(count));
        return true;
    };

    auto sendMany = [&](vector<string> ids, int dest) {
        sort(ids.begin(), ids.end(), [&](const string& a, const string& b) {
            return mapDist(warriors[a].zone, dest) < mapDist(warriors[b].zone, dest);
        });
        for (const string& id : ids) safeMove(id, dest);
    };

    ThreatInfo threat = analyzeThreat(day);
    if (threat.hqEmergency && (threat.massHqRush || threat.enemyAtHQ > 0 || lastTurnMyHqSieged)) {
        hqPressure = false;
    }
    currentAttackTarget = chooseAttackTarget(threat, day);
    rallyZone = chooseRallyZone(currentAttackTarget);

    int earlyGoal = min(4, max(2, (int)safeStrongholds.size()));
    int reachableEcoGoal = 0;
    for (int z : safeStrongholds) {
        if (distMy[z] >= INF / 2 || distOpp[z] >= INF / 2) continue;
        bool nearEnough = (hqDistance >= INF / 2) || distLePercent(distMy[z], hqDistance, 62);
        if (nearEnough && distLePercent(distMy[z], distOpp[z], 115)) reachableEcoGoal++;
    }
    int desiredGoal = min(10, max(earlyGoal, reachableEcoGoal));
    int coreEcoGoal = min(desiredGoal, max(earlyGoal, min(8, reachableEcoGoal)));
    auto expansionReserveGold = [&]() -> ll {
        if (threat.hqEmergency || myBaseCount() >= desiredGoal) return 0;
        for (int z : safeStrongholds) {
            if (!expansionBuildSiteAllowed(z, earlyGoal, desiredGoal)) continue;
            if (myIdleAt(z) > 0 || movingTo(meC, z) > 0) return BASE_BUILD_COST;
        }
        return 0;
    };

    int myHQLevelNow = hqLevelOf(meC);
    int oppHQLevelNow = hqLevelOf(oppC);
    int oppHQHpNow = hqHpOf(oppC);
    int myHQHpNow = hqHpOf(meC);
    bool losingHpRace = oppHQHpNow > myHQHpNow;
    bool lateHpRace = day >= 150 && losingHpRace;
    int hqTechTarget = 1;
    if (hasMyHQ() && !threat.hqEmergency && (myBaseCount() >= 3 || day >= 105)) {
        if (day >= 80 && myBaseCount() >= 4) hqTechTarget = max(hqTechTarget, 2);
        if (day >= 105 && myBaseCount() >= 6) hqTechTarget = max(hqTechTarget, 3);
        if (day >= 120 && myBaseCount() >= 5) hqTechTarget = max(hqTechTarget, 3);
        if (day >= 128 && myBaseCount() >= min(coreEcoGoal, 7) &&
            (losingHpRace || oppHQLevelNow >= 3 || oppHQHpNow >= 20)) {
            hqTechTarget = max(hqTechTarget, 4);
        }
        if (day >= 145 && myBaseCount() >= min(coreEcoGoal, 7) &&
            (losingHpRace || oppHQLevelNow >= 4 || oppHQHpNow >= 25)) {
            hqTechTarget = max(hqTechTarget, 5);
        }
        if (day >= 165 && myHQHpNow < oppHQHpNow) hqTechTarget = max(hqTechTarget, 5);
        if (day >= 185 && myHQHpNow < oppHQHpNow) hqTechTarget = max(hqTechTarget, 5);
    }
    bool needHQTech = hasMyHQ() && !threat.hqEmergency && myHQLevelNow < hqTechTarget;
    bool finishPressure = hqPressure && !threat.hqEmergency && day >= 125;
    int hqMassRallyCount = 0;
    ll hqMassLaunchCost = 0;
    bool hqMassRushPlan = false;
    bool hqMassRushReady = false;
    bool hqMassRushSaving = false;
    if (hqPressure && currentAttackTarget == oppHQ && !threat.hqEmergency &&
        validZone(rallyZone) && rallyZone != oppHQ && day >= 145 && day < 185) {
        int committedHQ = myAt(oppHQ) + movingTo(meC, oppHQ);
        int req = attackRequirement(oppHQ, day);
        int threshold = max(24, req);
        bool armyLead = countWarriors(meC) >= countWarriors(oppC) + 8 || day >= 170;
        hqMassRallyCount = myIdleAt(rallyZone);
        int costPer = moveCostTo(oppHQ);
        hqMassLaunchCost = 1LL * hqMassRallyCount * costPer;
        hqMassRushPlan = committedHQ == 0 && armyLead && hqMassRallyCount >= threshold;
        hqMassRushReady = hqMassRushPlan && availableGold >= hqMassLaunchCost;
        hqMassRushSaving = hqMassRushPlan && !hqMassRushReady;
        if (hqMassRushSaving) protectedMoveReserve = max(protectedMoveReserve, hqMassLaunchCost);
    }
    if (needHQTech && !finishPressure && hqLevelOf(meC) < hqTechTarget &&
        (myBaseCount() >= coreEcoGoal || day >= 120)) {
        protectedMoveReserve = HQ_UPGRADE_COST[hqLevelOf(meC) + 1];
    }


    if (threat.hqEmergency) {
        auto it = buildings.find(myHQ);
        int level = hqLevelOf(meC);
        bool urgentHQTech = level < 5 && (threat.massHqRush || day >= 175 ||
                            (it != buildings.end() && it->second.hp <= HQ_MAX_HP[level] * 2 / 3));
        if (it != buildings.end() && urgentHQTech && availableGold >= HQ_UPGRADE_COST[level + 1] + 40) {
            safeUpgrade(myHQ);
        } else if (it != buildings.end() && it->second.hp <= HQ_MAX_HP[level] * 2 / 3) {
            safeUpgrade(myHQ);
        }
    }


    if (!hqMassRushPlan && !threat.hqEmergency && (!hqPressure || myBaseCount() < 2) &&
        myBaseCount() < desiredGoal) {
        vector<int> buildCandidates;
        for (int z : safeStrongholds) {
            if (!expansionBuildSiteAllowed(z, earlyGoal, desiredGoal)) continue;
            if (myIdleAt(z) <= 0 || oppAt(z) > 0) continue;
            buildCandidates.push_back(z);
        }
        sort(buildCandidates.begin(), buildCandidates.end(), [&](int a, int b) {
            if (distMy[a] != distMy[b]) return distMy[a] < distMy[b];
            return llabs(distMy[a] - distOpp[a]) < llabs(distMy[b] - distOpp[b]);
        });
        for (int z : buildCandidates) {
            bool urgent = myBaseCount() < earlyGoal ||
                          (myBaseCount() < coreEcoGoal && day <= 125);
            ll reserveForTrain = urgent ? 0 : TRAIN_COST;
            if (needHQTech && !urgent && myBaseCount() >= coreEcoGoal) {
                reserveForTrain += HQ_UPGRADE_COST[min(5, myHQLevelNow + 1)];
            }
            if (availableGold < BASE_BUILD_COST + reserveForTrain) break;
            safeUpgrade(z);
        }
    }

    if (!hqMassRushPlan && needHQTech && hasMyHQ() && myIdleAt(myHQ) > 0 &&
        !upgradedToday.count(myHQ) && oppAt(myHQ) == 0) {
        int level = hqLevelOf(meC);
        if (level < hqTechTarget) {
            ll reserveAfter = (level < 3) ? (day >= 115 ? 40 : 100) : (lateHpRace || day >= 140 ? 40 : 180);
            if (availableGold >= HQ_UPGRADE_COST[level + 1] + reserveAfter) {
                safeUpgrade(myHQ);
                myHQLevelNow = hqLevelOf(meC);
            }
        }
    }


    {
        int level = commandStartHQLevel;
        int cap = HQ_TRAIN_CAP[level];
        ll buildReserve = expansionReserveGold();
        ll hqReserve = 0;
        if (needHQTech && !finishPressure && hqLevelOf(meC) < hqTechTarget) {
            int level = hqLevelOf(meC);
            ll nextCost = HQ_UPGRADE_COST[level + 1];
            bool cheapTech = level < 3;
            bool fullReserve = cheapTech || myBaseCount() >= desiredGoal ||
                               (level >= 4 && day >= 175 && myHQHpNow < oppHQHpNow) ||
                               (lateHpRace && day >= 165);
            hqReserve = fullReserve ? nextCost : nextCost * 2 / 3;
            if (level >= 3 && countWarriors(meC) + 6 < countWarriors(oppC)) {
                hqReserve = min(hqReserve, nextCost / 3);
            }
        }
        int affordable = (availableGold > buildReserve + hqReserve) ?
                         (int)((availableGold - buildReserve - hqReserve) / TRAIN_COST) : 0;
        int want = min(cap, affordable);
        if (hqMassRushPlan) want = 0;
        int laborCap = 0;
        for (const auto& kv : buildings) {
            if (kv.second.owner == meC) laborCap += buildingLaborCap(kv.second);
        }
        bool economyTooThin = !threat.defenseMode && !hqPressure &&
                              countWarriors(meC) > laborCap + 8 && myBaseCount() < 2;
        if (economyTooThin) want = min(want, 1);
        safeTrain(want);
    }

    if (!threat.hqEmergency && !hqMassRushPlan) {
        if (!needHQTech && myBaseCount() >= 3 && myIdleAt(myHQ) > 0) {
            int level = hqLevelOf(meC);
            if (level < 3 && !upgradedToday.count(myHQ) && availableGold >= HQ_UPGRADE_COST[level + 1] + 180) {
                safeUpgrade(myHQ);
            } else if (level < 5 && !upgradedToday.count(myHQ) && availableGold >= HQ_UPGRADE_COST[level + 1] + (lateHpRace ? 200 : 1500)) {
                safeUpgrade(myHQ);
            }
        }

        auto tryBaseUpgrade = [&](int z, ll reserve) {
            auto it = buildings.find(z);
            if (it == buildings.end()) return;
            const Building& b = it->second;
            if (b.owner != meC || b.isHQ) return;
            if (myIdleAt(z) <= 0 || oppAt(z) > 0) return;
            if (b.level >= 3 && b.hp >= BASE_MAX_HP[3]) return;
            ll cost = (b.level < 3) ? BASE_UPGRADE_COST[b.level + 1] : BASE_REPAIR_COST;
            if (availableGold >= cost + reserve) safeUpgrade(z);
        };

        if (!needHQTech) {
            if (validZone(rallyZone) && rallyZone != myHQ) {
                tryBaseUpgrade(rallyZone, hqPressure ? 80 : 220);
            }
            for (const auto& kv : buildings) {
                int z = kv.first;
                const Building& b = kv.second;
                if (b.owner != meC || b.isHQ || z == rallyZone) continue;
                if (distMy[z] > distOpp[z] && day < 140) continue;
                if (plannedMyStaff(z) >= buildingLaborCap(b)) {
                    tryBaseUpgrade(z, 420);
                }
            }
        }
    }

    set<string> reserved = reserveWorkers(threat);
    if (hqMassRushPlan) {
        for (const string& id : idleIdsAt(rallyZone)) reserved.erase(id);
    }
    if (needHQTech && !finishPressure && !hqMassRushPlan) {
        for (const string& id : idleIdsAt(myHQ)) reserved.insert(id);
    }

    if (needHQTech && !finishPressure && !hqMassRushPlan &&
        myIdleAt(myHQ) == 0 && movingTo(meC, myHQ) == 0) {
        vector<string> techIds = freeWarriors(reserved, day >= 125);
        auto bestIt = min_element(techIds.begin(), techIds.end(), [&](const string& a, const string& b) {
            return mapDist(warriors[a].zone, myHQ) < mapDist(warriors[b].zone, myHQ);
        });
        if (bestIt != techIds.end() && safeMove(*bestIt, myHQ)) reserved.insert(*bestIt);
    }

    if (!threat.defenseMode && myBaseCount() < desiredGoal) {
        for (int z : safeStrongholds) {
            if (!expansionBuildSiteAllowed(z, earlyGoal, desiredGoal)) continue;
            vector<string> ids = idleIdsAt(z);
            if (!ids.empty()) reserved.insert(ids.front());
        }
    }

    if (threat.defenseMode) {
        bool releaseWorkers = threat.hqEmergency;
        vector<string> defenders = freeWarriors(reserved, releaseWorkers);
        int have = myAt(threat.defenseTarget) + movingTo(meC, threat.defenseTarget);
        sort(defenders.begin(), defenders.end(), [&](const string& a, const string& b) {
            return mapDist(warriors[a].zone, threat.defenseTarget) <
                   mapDist(warriors[b].zone, threat.defenseTarget);
        });
        for (const string& id : defenders) {
            if (have >= threat.guardNeed) break;
            if (warriors[id].zone == threat.defenseTarget) {
                have++;
                continue;
            }
            if (safeMove(id, threat.defenseTarget)) have++;
        }
    }

    if (threat.defenseMode && (!hqPressure || threat.hqEmergency)) {
        vector<string> rest = freeWarriors(reserved, threat.hqEmergency);
        sendMany(rest, threat.defenseTarget);
    } else {

        if (!hqPressure && !threat.hqEmergency && myBaseCount() < desiredGoal) {
            vector<string> freeIds = freeWarriors(reserved, false);
            vector<int> targets;
            for (int z : safeStrongholds) {
                if (safeExpansionZone(z, earlyGoal, desiredGoal)) targets.push_back(z);
            }
            sort(targets.begin(), targets.end(), [&](int a, int b) {
                if (distMy[a] != distMy[b]) return distMy[a] < distMy[b];
                return llabs(distMy[a] - distOpp[a]) < llabs(distMy[b] - distOpp[b]);
            });
            int issued = 0;
            int issueLimit = (myBaseCount() < coreEcoGoal) ? 3 : 2;
            for (int z : targets) {
                if (issued >= issueLimit) break;
                vector<string> nowFree = freeWarriors(reserved, false);
                if (nowFree.empty()) break;
                auto bestIt = min_element(nowFree.begin(), nowFree.end(), [&](const string& a, const string& b) {
                    return mapDist(warriors[a].zone, z) < mapDist(warriors[b].zone, z);
                });
                if (bestIt != nowFree.end() && safeMove(*bestIt, z)) issued++;
            }
        }

        vector<string> freeIds = freeWarriors(reserved, false);
        bool launch = shouldLaunchAttack(currentAttackTarget, rallyZone, day, freeIds);
        if (hqPressure && currentAttackTarget == oppHQ) {
            currentAttackTarget = oppHQ;
            launch = shouldLaunchAttack(currentAttackTarget, rallyZone, day, freeIds);
        }
        if (hqMassRushSaving) launch = false;
        if (hqMassRushReady) launch = true;

        if (launch) {
            if (hqMassRushReady && currentAttackTarget == oppHQ) {
                vector<string> rallyIds;
                vector<string> restIds;
                for (const string& id : freeIds) {
                    auto it = warriors.find(id);
                    if (it != warriors.end() && it->second.zone == rallyZone) rallyIds.push_back(id);
                    else restIds.push_back(id);
                }
                sendMany(rallyIds, currentAttackTarget);
                sendMany(restIds, currentAttackTarget);
            } else {

                sendMany(freeIds, currentAttackTarget);
            }
        } else {

            sendMany(freeIds, rallyZone);
        }
    }

    cout << "COMMAND\n";
    for (const string& cmd : commands) cout << cmd << '\n';
    cout << "END\n" << flush;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string cmd;
    while (cin >> cmd) {
        if (cmd == "READY") {
            parseReady();
        } else if (cmd == "START") {
            string word;
            int day;
            cin >> word >> day;
            doTurn(day);
        } else if (cmd == "TURN") {
            parseTurnResult();
        } else if (cmd == "FINISH") {
            break;
        } else if (cmd == "RESULT") {
            string rest;
            getline(cin, rest);
            break;
        }
    }
    return 0;
}
