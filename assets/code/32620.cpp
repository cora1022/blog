#include <bits/stdc++.h>
using namespace std;


static const int HQ_UP[6]    = {0, 0, 600, 1200, 2400, 3600};
static const int HQ_REPAIR   = 1000;
static const int HQ_WHP[6]   = {0, 4, 5, 6, 7, 8};
static const int HQ_HP[6]    = {0, 10, 15, 20, 25, 30};
static const int HQ_TUR[6]   = {0, 1, 2, 2, 3, 3};
static const int HQ_TRAIN[6] = {0, 1, 1, 2, 2, 3};
static const int HQ_LAB[6]   = {0, 1, 2, 3, 4, 5};
static const int BS_COST[4]  = {0, 300, 600, 1000};
static const int BS_REPAIR   = 500;
static const int BS_HP[4]    = {0, 6, 12, 18};
static const int BS_TUR[4]   = {0, 1, 1, 2};
static const int BS_LAB[4]   = {0, 1, 2, 3};

static const long long GOLD_BUFFER = 20;
static const double EXPAND_REACH = 1.30;
static const int ATTACK_SIZE = 6;


int N, K;
vector<long long> PX, PY;
vector<char> isBase;
vector<vector<int>> adj;
vector<vector<double>> dist;
vector<int> strongOrder;
vector<int> hopHQ;
int baseCap = 0;
int centerZone = -1;

char meC, oppC;
int myHQ, oppHQ;


long long gold = 500;
int byoyomi = 5;

struct War { char owner; int zone; int hp; bool moving; int target; };
map<string, War> war;
struct Bld { char owner; int level; int hp; bool hq; };
map<int, Bld> bld;

int hqZoneOf(char c) { return c == 'A' ? 0 : N - 1; }
int hqLevelOf(char c) { auto it = bld.find(hqZoneOf(c)); return it == bld.end() ? 1 : it->second.level; }

int myInZone(int z) { int c = 0; for (auto& kv : war) if (kv.second.owner == meC && kv.second.zone == z) c++; return c; }
int oppInZone(int z) { int c = 0; for (auto& kv : war) if (kv.second.owner == oppC && kv.second.zone == z) c++; return c; }
int myCount() { int c = 0; for (auto& kv : war) if (kv.second.owner == meC) c++; return c; }

int bldMaxHP(const Bld& b) { return b.hq ? HQ_HP[b.level] : BS_HP[b.level]; }
int bldLabCap(const Bld& b) { return b.hq ? HQ_LAB[b.level] : BS_LAB[b.level]; }

int totalWorkCap() {
    int s = 0;
    for (auto& kv : bld) if (kv.second.owner == meC) s += bldLabCap(kv.second);
    return s;
}


int countThreat() {
    int t = 0;
    for (auto& kv : war)
        if (kv.second.owner == oppC) {
            int z = kv.second.zone;
            if (z >= 0 && z < N && dist[myHQ][z] <= dist[oppHQ][z]) t++;
        }
    return t;
}


void parseReady() {
    string side; cin >> side;
    meC = (side == "LEFT") ? 'A' : 'B';
    oppC = (meC == 'A') ? 'B' : 'A';
    cin >> N >> K;
    PX.assign(N, 0); PY.assign(N, 0);
    for (int i = 0; i < N; i++) cin >> PX[i];
    for (int i = 0; i < N; i++) cin >> PY[i];
    isBase.assign(N, 0);
    for (int k = 0; k < K; k++) { int p; cin >> p; isBase[p] = 1; }
    adj.assign(N, {});
    for (int i = 0; i < N; i++) {
        int a; cin >> a;
        adj[i].resize(a);
        for (int j = 0; j < a; j++) cin >> adj[i][j];
    }
    myHQ = hqZoneOf(meC); oppHQ = hqZoneOf(oppC);
    bld[0]     = {'A', 1, HQ_HP[1], true};
    bld[N - 1] = {'B', 1, HQ_HP[1], true};
    for (int i = 1; i <= 3; i++) {
        war[string("A") + to_string(i)] = {'A', 0,     4, false, -1};
        war[string("B") + to_string(i)] = {'B', N - 1, 4, false, -1};
    }

    const double INF = 1e18;
    dist.assign(N, vector<double>(N, INF));
    for (int i = 0; i < N; i++) dist[i][i] = 0;
    for (int u = 0; u < N; u++)
        for (int v : adj[u]) {
            double dx = (double)(PX[u] - PX[v]), dy = (double)(PY[u] - PY[v]);
            double w = ceil(sqrt(dx * dx + dy * dy));
            if (w < dist[u][v]) dist[u][v] = w;
        }
    for (int k = 0; k < N; k++)
        for (int i = 0; i < N; i++) {
            if (dist[i][k] >= INF) continue;
            for (int j = 0; j < N; j++)
                if (dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
        }
    hopHQ.assign(N, 1000000000);
    { queue<int> q; q.push(myHQ); hopHQ[myHQ] = 0;
      while (!q.empty()) { int u = q.front(); q.pop();
          for (int v : adj[u]) if (hopHQ[v] > hopHQ[u] + 1) { hopHQ[v] = hopHQ[u] + 1; q.push(v); } } }
    centerZone = 0;
    for (int z = 1; z < N; z++)
        if (llabs(PX[z]) + llabs(PY[z]) < llabs(PX[centerZone]) + llabs(PY[centerZone]))
            centerZone = z;
    for (int z = 0; z < N; z++) if (isBase[z]) strongOrder.push_back(z);
    sort(strongOrder.begin(), strongOrder.end(), [&](int a, int b) {
        if (hopHQ[a] != hopHQ[b]) return hopHQ[a] < hopHQ[b];
        if (fabs(dist[myHQ][a] - dist[myHQ][b]) > 1e-7) return dist[myHQ][a] < dist[myHQ][b];
        return dist[a][centerZone] < dist[b][centerZone];
    });
    baseCap = 0;
    for (int z : strongOrder) if (dist[myHQ][z] <= dist[oppHQ][z] * EXPAND_REACH) baseCap++;

    gold = 500; byoyomi = 5;
    cout << "OK\n" << flush;
}


void doCommands(int day) {
    (void)day;
    cout << "COMMAND\n";
    if (!bld.count(myHQ)) { cout << "END\n" << flush; return; }
    long long avail = gold - GOLD_BUFFER;
    int hl = bld[myHQ].level;


    int eField = 0, spearHop = INT_MAX, spearZone = -1;
    for (auto& kv : war) {
        if (kv.second.owner != oppC) continue;
        int z = kv.second.zone;
        if (z < 0 || z >= N) continue;
        bool worker = bld.count(z) && bld[z].owner == oppC;
        if (worker) continue;
        eField++;
        if (hopHQ[z] < spearHop) { spearHop = hopHQ[z]; spearZone = z; }
    }
    int hqTur = HQ_TUR[hl];
    int guardNeed = max(0, eField - hqTur) + 1;
    int hqHopWidth = (oppHQ >= 0 && oppHQ < (int)hopHQ.size()) ? hopHQ[oppHQ] : 1000000000;


    int naturalHold = hqTur + HQ_LAB[hl];
    bool menace = false;
    if (eField > naturalHold && spearZone >= 0) {
        if (dist[myHQ][spearZone] <= dist[oppHQ][spearZone]) menace = true;
        else if (hqHopWidth < 1000000000 && spearHop * 10 <= hqHopWidth * 6) menace = true;
    }

    set<int> usedUpg;

    int nBases = 0;
    for (auto& kv : bld) if (kv.second.owner == meC && !kv.second.hq) nBases++;
    bool expandDone = (nBases >= baseCap);
    bool econOK = expandDone || nBases >= 4;


    auto staffOf = [&](int z) {
        int c = 0;
        for (auto& kv : war)
            if (kv.second.owner == meC &&
                (kv.second.zone == z || (kv.second.moving && kv.second.target == z))) c++;
        return c;
    };


    for (auto& kv : war) {
        if (gold < 300) break;
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        if (z == myHQ || !isBase[z] || bld.count(z) || usedUpg.count(z)) continue;
        if (oppInZone(z) > 0) continue;
        cout << "UPGRADE " << z << "\n";
        gold -= 300; avail -= 300; usedUpg.insert(z);
        bld[z] = {meC, 1, BS_HP[1], false}; nBases++;
    }


    if (oppInZone(myHQ) == 0 && myInZone(myHQ) >= 1 && !usedUpg.count(myHQ)) {
        if (hl < 5 && !menace) {
            long long need = HQ_UP[hl + 1];
            long long buffer = (hl < 3) ? 240 : 1500;
            if (econOK && avail >= need + buffer) {
                cout << "UPGRADE " << myHQ << "\n";
                gold -= need; avail -= need; usedUpg.insert(myHQ);
                bld[myHQ].level = ++hl; bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (bld[myHQ].hp <= HQ_HP[5] - 5 && avail >= HQ_REPAIR) {
            cout << "UPGRADE " << myHQ << "\n";
            gold -= HQ_REPAIR; avail -= HQ_REPAIR; usedUpg.insert(myHQ);
            bld[myHQ].hp = HQ_HP[5];
        }
    }


    for (auto& kv : bld) {
        if (menace) break;
        Bld& b = kv.second; int z = kv.first;
        if (b.owner != meC || b.hq || b.level >= 2) continue;
        if (usedUpg.count(z) || oppInZone(z) > 0 || myInZone(z) < bldLabCap(b)) continue;
        long long c = BS_COST[b.level + 1];
        if (avail >= c + 800) {
            cout << "UPGRADE " << z << "\n";
            gold -= c; avail -= c; usedUpg.insert(z);
            b.level++; b.hp = BS_HP[b.level];
        }
    }


    {

        long long baseReserve = 0;
        for (auto& kv : war) {
            if (kv.second.owner != meC) continue;
            int tg = kv.second.moving ? kv.second.target : kv.second.zone;
            if (tg >= 0 && tg < N && tg != myHQ && isBase[tg] &&
                !(bld.count(tg) && bld[tg].owner == meC) && oppInZone(tg) == 0)
                baseReserve += 300;
        }
        if (baseReserve > 900) baseReserve = 900;

        if (menace) baseReserve = 0;
        int cap = HQ_TRAIN[hl];
        int twc = totalWorkCap();
        int units = myCount();
        int sustain = 6 * max(1, twc);
        long long trainable = avail - baseReserve;
        int want = 0;
        if (trainable > 0 && units < sustain) want = min(cap, (int)(trainable / 120));


        if (nBases < 2) want = 0;
        while (want > 0 && 120LL * want > avail) want--;
        if (want > 0) { cout << "TRAIN " << want << "\n"; gold -= 120LL * want; avail -= 120LL * want; }
    }


    auto sendTo = [&](const string& w, int dest) {
        cout << "MOVE " << w << " " << dest << "\n";
        long long mc = (bld.count(dest) && bld[dest].owner == meC) ? 0 : 10;
        gold -= mc; avail -= mc; war[w].moving = true; war[w].target = dest;
    };


    map<int,int> kept;
    vector<string> freeW;
    for (auto& kv : war) {
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        if (z == myHQ) {
            int lim = max(1, HQ_LAB[hl]);
            if (menace) lim = max(lim, guardNeed);
            if (kept[z] < lim) { kept[z]++; continue; }
        } else if (bld.count(z) && bld[z].owner == meC) {
            if (kept[z] < 1) { kept[z]++; continue; }
        } else if (isBase[z]) {
            bool enemyBldHere = bld.count(z) && bld[z].owner == oppC;
            bool winning = myInZone(z) > oppInZone(z);

            if (enemyBldHere || (oppInZone(z) > 0 && winning)) { kept[z]++; continue; }

            if (oppInZone(z) == 0 && !bld.count(z) &&
                dist[myHQ][z] <= dist[oppHQ][z] * EXPAND_REACH) {
                if (kept[z] < 1) { kept[z]++; continue; }
            }

        }
        freeW.push_back(kv.first);
    }

    sort(freeW.begin(), freeW.end(), [&](const string& a, const string& b) {
        return dist[myHQ][war[a].zone] < dist[myHQ][war[b].zone];
    });
    size_t fi = 0;


    for (auto& kv : bld) {
        if (fi >= freeW.size()) break;
        if (kv.second.owner != meC || kv.second.hq) continue;
        int z = kv.first;
        if (oppInZone(z) > 0 || staffOf(z) >= 1) continue;
        sendTo(freeW[fi++], z);
    }


    if (menace) {
        int have = staffOf(myHQ);
        while (fi < freeW.size() && have < guardNeed) {
            if (war[freeW[fi]].zone == myHQ) { fi++; continue; }
            sendTo(freeW[fi++], myHQ); have++;
        }
        if (have < guardNeed) {


            vector<pair<int,string>> rec;
            for (auto& kv : war) {
                if (kv.second.owner != meC || kv.second.moving) continue;
                int z = kv.second.zone;
                if (z == myHQ || z < 0 || z >= N) continue;
                bool ourHalf = dist[myHQ][z] <= dist[oppHQ][z];
                if (ourHalf && spearHop > 5) continue;
                rec.push_back({hopHQ[z], kv.first});
            }
            sort(rec.begin(), rec.end(), [](const pair<int,string>& a, const pair<int,string>& b) {
                return a.first < b.first;
            });
            for (auto& pr : rec) { if (have >= guardNeed) break; sendTo(pr.second, myHQ); have++; }
        }
    }


    for (int z : strongOrder) {
        if (fi >= freeW.size() || avail < 10) break;
        if (bld.count(z)) continue;
        if (dist[myHQ][z] > dist[oppHQ][z] * EXPAND_REACH) continue;
        if (oppInZone(z) > 0) continue;
        if (staffOf(z) >= 1) continue;
        sendTo(freeW[fi++], z);
    }


    if (fi < freeW.size()) {
        auto turretOf = [&](int z) -> int {
            if (bld.count(z)) { Bld& b = bld[z]; return b.hq ? HQ_TUR[b.level] : BS_TUR[b.level]; }
            return 0;
        };

        int baseTgt = -1; double baseTgtD = 1e18;
        for (auto& kv : bld)
            if (kv.second.owner == oppC && !kv.second.hq && dist[myHQ][kv.first] < 1e17)
                if (dist[myHQ][kv.first] < baseTgtD) { baseTgtD = dist[myHQ][kv.first]; baseTgt = kv.first; }

        int clTgt = -1, clE = 0;
        for (int z = 0; z < N; z++) {
            if (bld.count(z)) continue;
            int e = oppInZone(z);
            if (e > 0 && dist[myHQ][z] <= dist[oppHQ][z] && e > clE) { clE = e; clTgt = z; }
        }


        int rallyZone = myHQ; { double rb = 1e18;
            for (auto& kv : bld) if (kv.second.owner == meC) {
                double d = dist[kv.first][oppHQ];
                if (d < rb) { rb = d; rallyZone = kv.first; } } }


        auto reinforceTo = [&](int dest, int req) {
            int have = staffOf(dest);
            while (fi < freeW.size() && avail >= 10 && have < req) {
                if (war[freeW[fi]].zone == dest) { fi++; continue; }
                sendTo(freeW[fi++], dest); have++;
            }
        };

        bool launched = false;


        if (menace) {
            if (dist[myHQ][oppHQ] < 1e17) {
                int req = max(ATTACK_SIZE, oppInZone(oppHQ) + turretOf(oppHQ) + 3);
                int force = (int)(freeW.size() - fi) + staffOf(oppHQ);
                if (force >= req || staffOf(oppHQ) > 0) {
                    while (fi < freeW.size() && avail >= 10) {
                        if (war[freeW[fi]].zone == oppHQ) { fi++; continue; }
                        sendTo(freeW[fi++], oppHQ);
                    }
                }
            }
            launched = true;
        }


        if (!launched && baseTgt >= 0) {
            int blvl = bld.count(baseTgt) ? bld[baseTgt].level : 1;
            int req = max(2, oppInZone(baseTgt) + blvl + 1);
            int force = (int)(freeW.size() - fi) + staffOf(baseTgt);
            if (force >= req || staffOf(baseTgt) > 0) { reinforceTo(baseTgt, req); launched = true; }
        }

        if (!launched && clTgt >= 0) {
            int req = clE + 1;
            int force = (int)(freeW.size() - fi) + staffOf(clTgt);
            if (force >= req || staffOf(clTgt) > 0) { reinforceTo(clTgt, req); launched = true; }
        }

        if (!launched && !menace && baseTgt < 0 && clTgt < 0 && dist[myHQ][oppHQ] < 1e17) {
            int req = max(ATTACK_SIZE, oppInZone(oppHQ) + turretOf(oppHQ) + 3);
            int force = (int)(freeW.size() - fi) + staffOf(oppHQ);
            if (force >= req || staffOf(oppHQ) > 0) {
                while (fi < freeW.size() && avail >= 10) {
                    if (war[freeW[fi]].zone == oppHQ) { fi++; continue; }
                    sendTo(freeW[fi++], oppHQ);
                }
                launched = true;
            }
        }

        int funnelTo = menace ? myHQ : rallyZone;
        while (fi < freeW.size()) {
            if (war[freeW[fi]].zone == funnelTo) { fi++; continue; }
            if (funnelTo != myHQ && !(bld.count(funnelTo) && bld[funnelTo].owner == meC) && avail < 10) break;
            sendTo(freeW[fi++], funnelTo);
        }
    }

    cout << "END\n" << flush;
}


void parseResult() {
    int T; cin >> T;
    string sec; cin >> sec;
    long long Tx, Rx, Ty, Ry; cin >> Tx >> Rx >> Ty >> Ry;
    byoyomi = (int)Rx;

    vector<pair<char,int>> ups;
    vector<string> trains;
    vector<pair<string,int>> moves;
    vector<tuple<string,string,int>> dmg;
    vector<tuple<char,int,int>> sieges;

    while (cin >> sec) {
        if (sec == "END") break;
        int n; cin >> n;
        if (sec == "UPGRADE") {
            for (int i = 0; i < n; i++) { string t; int d; cin >> t >> d; ups.push_back({t[0], d}); }
        } else if (sec == "TRAIN") {
            for (int i = 0; i < n; i++) { string id; cin >> id; trains.push_back(id); }
        } else if (sec == "MOVE") {
            for (int i = 0; i < n; i++) { string id; int d; cin >> id >> d; moves.push_back({id, d}); }
        } else if (sec == "DAMAGE") {
            for (int i = 0; i < n; i++) { string c, id; int h; cin >> c >> id >> h; dmg.push_back({c, id, h}); }
        } else if (sec == "SIEGE") {
            for (int i = 0; i < n; i++) { string t; int d, h; cin >> t >> d >> h; sieges.push_back({t[0], d, h}); }
        } else {
            for (int i = 0; i < n; i++) { string junk; getline(cin, junk); }
        }
    }


    for (auto& [t, d] : ups) {
        if (t == meC) {
            if (!bld.count(d)) bld[d] = {meC, 1, BS_HP[1], false};
            continue;
        }
        if (!bld.count(d)) bld[d] = {t, 1, BS_HP[1], false};
        else {
            Bld& b = bld[d];
            int mx = b.hq ? 5 : 3;
            if (b.level < mx) b.level++;
            b.hp = bldMaxHP(b);
        }
    }

    for (auto& id : trains) {
        char ow = id[0];
        war[id] = {ow, hqZoneOf(ow), HQ_WHP[hqLevelOf(ow)], false, -1};
    }

    for (auto& [id, d] : moves) {
        auto it = war.find(id);
        if (it != war.end()) {
            it->second.zone = d;
            if (it->second.moving && it->second.zone == it->second.target)
                it->second.moving = false;
        }
    }

    for (auto& [c, id, h] : dmg) {
        if (c == "HUNGER") continue;
        auto it = war.find(id);
        if (it != war.end()) it->second.hp -= h;
    }
    for (auto& [t, d, h] : sieges) {
        auto it = bld.find(d);
        if (it != bld.end()) it->second.hp -= h;
    }
    for (auto it = war.begin(); it != war.end();) { if (it->second.hp <= 0) it = war.erase(it); else ++it; }
    for (auto it = bld.begin(); it != bld.end();) { if (it->second.hp <= 0) it = bld.erase(it); else ++it; }

    long long income = 0;
    for (auto& kv : bld) {
        if (kv.second.owner != meC) continue;
        income += 15LL * min(myInZone(kv.first), bldLabCap(kv.second));
    }
    gold += income;

    gold -= 2LL * myCount();
    if (gold < 0) gold = 0;
    for (auto& [c, id, h] : dmg) {
        if (c != "HUNGER") continue;
        auto it = war.find(id);
        if (it != war.end()) it->second.hp -= h;
    }
    for (auto it = war.begin(); it != war.end();) { if (it->second.hp <= 0) it = war.erase(it); else ++it; }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string cmd;
    while (cin >> cmd) {
        if (cmd == "READY") parseReady();
        else if (cmd == "START") { string tw; int T; cin >> tw >> T; doCommands(T); }
        else if (cmd == "TURN") parseResult();
        else if (cmd == "FINISH") break;
    }
    return 0;
}
