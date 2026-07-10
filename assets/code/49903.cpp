#include <bits/stdc++.h>
using namespace std;


struct Rules {

    int hqUpgrade[6] = {0, 0, 600, 1200, 2400, 3600};
    int hqRepair     = 1000;
    int hqUnitHp[6]  = {0, 4, 5, 6, 7, 8};
    int hqHp[6]      = {0, 10, 15, 20, 25, 30};
    int hqGun[6]     = {0, 1, 2, 2, 3, 3};
    int hqMint[6]    = {0, 1, 1, 2, 2, 3};
    int hqJob[6]     = {0, 1, 2, 3, 4, 5};

    int fcBuild[4]   = {0, 300, 600, 1000};
    int fcRepair     = 500;
    int fcHp[4]      = {0, 6, 12, 18};
    int fcGun[4]     = {0, 1, 1, 2};
    int fcJob[4]     = {0, 1, 2, 3};
} R;


struct Tune {
    long long safety   = 20;
    double    reach     = 1.30;
    int       fortGoal  = 99;
    int       fortTop   = 3;
    int       hqTop     = 5;
} CFG;


struct Soldier { char flag; int spot; int hp; bool walking; int goal; };
struct Citadel { char flag; int lvl; int hp; bool capital; };


struct World {
    int             cells = 0, fortSlots = 0;
    vector<double>  px, py;
    vector<char>    fortable;
    vector<vector<int>> nbr;
    vector<double>  reachMine, reachFoe;
    vector<int>     hopMine;
    vector<int>     fortByNear;
    int             quota = 0;
    int             core  = -1;

    char me = 'A', foe = 'B';
    int  capMine = 0, capFoe = 0;

    long long purse = 500;
    int       grace = 5;


    int  assaultUntil = 0;
    bool assaultOn    = false;


    int  fort1 = -1;
    int  fort2 = -1;


    int  lockTarget = -1;
    map<string, Soldier> army;
    map<int,    Citadel> base;


    int capitalCell(char f) const { return f == 'A' ? 0 : cells - 1; }
    int capitalLvl(char f) {
        auto it = base.find(capitalCell(f));
        return it == base.end() ? 1 : it->second.lvl;
    }
    int hereMine(int z) {
        int c = 0; for (auto& s : army) if (s.second.flag == me && s.second.spot == z) c++; return c;
    }
    int hereFoe(int z) {
        int c = 0; for (auto& s : army) if (s.second.flag == foe && s.second.spot == z) c++; return c;
    }
    int popMine() {
        int c = 0; for (auto& s : army) if (s.second.flag == me) c++; return c;
    }
    int citadelHp(const Citadel& b)  const { return b.capital ? R.hqHp[b.lvl]  : R.fcHp[b.lvl]; }
    int citadelJob(const Citadel& b) const { return b.capital ? R.hqJob[b.lvl] : R.fcJob[b.lvl]; }
    int citadelGun(const Citadel& b) const { return b.capital ? R.hqGun[b.lvl] : R.fcGun[b.lvl]; }
    int jobTotal() {
        int s = 0; for (auto& b : base) if (b.second.flag == me) s += citadelJob(b.second); return s;
    }
    bool mineOwns(int z) {
        auto it = base.find(z); return it != base.end() && it->second.flag == me;
    }
    bool inMyReach(int z) { return reachMine[z] <= reachFoe[z] * CFG.reach; }


    long long mightMine() {
        long long m = 0;
        for (auto& s : army) if (s.second.flag == me) { m += 1 + max(0, s.second.hp); }
        return m;
    }
    long long mightFoe() {
        long long m = 0;
        for (auto& s : army) if (s.second.flag == foe) { m += 1 + max(0, s.second.hp); }
        return m;
    }

    int boundFor(int z) {
        int c = 0;
        for (auto& s : army)
            if (s.second.flag == me &&
                (s.second.spot == z || (s.second.walking && s.second.goal == z))) c++;
        return c;
    }
} W;


static double edgeLen(int u, int v) {
    double dx = W.px[u] - W.px[v], dy = W.py[u] - W.py[v];
    return ceil(sqrt(dx * dx + dy * dy));
}
static vector<double> dijkstra(int src) {
    vector<double> dist(W.cells, 1e18);
    priority_queue<pair<double,int>, vector<pair<double,int>>, greater<>> pq;
    dist[src] = 0; pq.push({0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u] + 1e-9) continue;
        for (int v : W.nbr[u]) {
            double nd = d + edgeLen(u, v);
            if (nd < dist[v]) { dist[v] = nd; pq.push({nd, v}); }
        }
    }
    return dist;
}
static vector<int> bfsHops(int src) {
    vector<int> h(W.cells, INT_MAX);
    queue<int> q; q.push(src); h[src] = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : W.nbr[u]) if (h[v] > h[u] + 1) { h[v] = h[u] + 1; q.push(v); }
    }
    return h;
}


static int fortTarget() { return 2; }


static void order(const string& id, int dst, long long& budget) {
    auto it = W.army.find(id);
    if (it == W.army.end()) return;
    if (it->second.walking) return;
    if (it->second.spot == dst) return;
    long long fee = (W.base.count(dst) && W.base[dst].flag == W.me) ? 0 : 10;


    if (fee > 0 && W.purse < fee) return;
    cout << "MOVE " << id << " " << dst << "\n";
    W.purse -= fee; budget -= fee;
    it->second.walking = true; it->second.goal = dst;
}


static void bootstrap() {
    string lr; cin >> lr;
    W.me  = (lr == "LEFT") ? 'A' : 'B';
    W.foe = (W.me == 'A') ? 'B' : 'A';
    cin >> W.cells >> W.fortSlots;

    W.px.assign(W.cells, 0); W.py.assign(W.cells, 0);
    for (int i = 0; i < W.cells; i++) cin >> W.px[i];
    for (int i = 0; i < W.cells; i++) cin >> W.py[i];

    W.fortable.assign(W.cells, 0);
    for (int k = 0; k < W.fortSlots; k++) { int p; cin >> p; W.fortable[p] = 1; }

    W.nbr.assign(W.cells, {});
    for (int i = 0; i < W.cells; i++) {
        int d; cin >> d; W.nbr[i].resize(d);
        for (int j = 0; j < d; j++) cin >> W.nbr[i][j];
    }

    W.capMine = W.capitalCell(W.me);
    W.capFoe  = W.capitalCell(W.foe);

    W.base[0]            = {'A', 1, R.hqHp[1], true};
    W.base[W.cells - 1]  = {'B', 1, R.hqHp[1], true};
    for (int i = 1; i <= 3; i++) {
        W.army["A" + to_string(i)] = {'A', 0,             4, false, -1};
        W.army["B" + to_string(i)] = {'B', W.cells - 1,   4, false, -1};
    }


    W.reachMine = dijkstra(W.capMine);
    W.reachFoe  = dijkstra(W.capFoe);
    W.hopMine   = bfsHops(W.capMine);


    W.core = 0;
    for (int z = 1; z < W.cells; z++)
        if (llabs((long long)W.px[z]) + llabs((long long)W.py[z]) <
            llabs((long long)W.px[W.core]) + llabs((long long)W.py[W.core]))
            W.core = z;

    for (int z = 0; z < W.cells; z++) if (W.fortable[z]) W.fortByNear.push_back(z);
    sort(W.fortByNear.begin(), W.fortByNear.end(), [&](int a, int b) {
        if (W.hopMine[a] != W.hopMine[b]) return W.hopMine[a] < W.hopMine[b];
        if (fabs(W.reachMine[a] - W.reachMine[b]) > 1e-7) return W.reachMine[a] < W.reachMine[b];
        return false;
    });

    W.quota = 0;
    for (int z : W.fortByNear) if (W.inMyReach(z)) W.quota++;


    {
        vector<int> reach;
        for (int z : W.fortByNear) if (W.inMyReach(z)) reach.push_back(z);
        if (reach.size() >= 1) W.fort1 = reach[0];
        if (reach.size() >= 2) W.fort2 = reach[1];

        if (W.fort2 < 0) W.fort2 = W.fort1;
    }

    W.purse = 500; W.grace = 5;
    cout << "OK\n" << flush;
}


struct Danger {
    int   fieldFoe = 0;
    int   leadHop  = INT_MAX;
    int   leadCell = -1;
    int   guard    = 0;
    bool  besieged = false;

    int   foeAtHQ    = 0;
    int   foeNearHQ  = 0;
    int   foeMyHalf  = 0;
    int   clusterCell= -1;
    int   clusterN   = 0;
    int   hitBase    = -1;
    int   defendCell = -1;
    bool  hqEmergency= false;
    bool  massRush   = false;
};

static Danger assessDanger(int hqLvl, int day) {
    Danger D;
    D.defendCell = W.capMine;


    double hqSpan = W.reachFoe[W.capMine];
    auto within = [&](int z, int pct) {
        if (hqSpan >= 1e17) return false;
        return W.reachMine[z] <= hqSpan * pct / 100.0;
    };

    vector<int> foeByCell(W.cells, 0);
    for (auto& s : W.army) {
        if (s.second.flag != W.foe) continue;
        int z = s.second.spot;
        if (z < 0 || z >= W.cells) continue;
        foeByCell[z]++;
        bool stationed = W.base.count(z) && W.base[z].flag == W.foe;
        if (!stationed) {
            D.fieldFoe++;
            if (W.hopMine[z] < D.leadHop) { D.leadHop = W.hopMine[z]; D.leadCell = z; }
        }
        if (z == W.capMine) D.foeAtHQ++;
        if (within(z, 35)) D.foeNearHQ++;
        if (W.reachMine[z] <= W.reachFoe[z] || within(z, 55)) D.foeMyHalf++;
    }


    for (int z = 0; z < W.cells; z++) {
        if (foeByCell[z] == 0) continue;
        bool inMyHalf = (W.reachMine[z] <= W.reachFoe[z]) || within(z, 60);
        if (!inMyHalf) continue;
        if (foeByCell[z] > D.clusterN ||
            (foeByCell[z] == D.clusterN && D.clusterCell >= 0 &&
             W.reachMine[z] < W.reachMine[D.clusterCell])) {
            D.clusterN = foeByCell[z]; D.clusterCell = z;
        }
    }

    double bestD = 1e18;
    for (auto& b : W.base) {
        if (b.second.flag != W.me || b.second.capital) continue;
        int z = b.first;
        if (W.hereFoe(z) == 0) continue;
        if (W.reachMine[z] < bestD) { bestD = W.reachMine[z]; D.hitBase = z; }
    }


    if (D.clusterCell >= 0) {
        bool bigNear   = D.clusterN >= 5  && within(D.clusterCell, 45);
        bool hugeMid   = D.clusterN >= 8  && within(D.clusterCell, 55);
        bool hugeDeep  = D.clusterN >= 10 && within(D.clusterCell, 65);
        bool lateAny   = day >= 150 && D.clusterN >= 5 && within(D.clusterCell, 55);
        D.massRush = bigNear || hugeMid || hugeDeep || lateAny;
    }

    int myHqHp = W.base.count(W.capMine) ? W.base[W.capMine].hp : 0;
    bool hqLow = myHqHp > 0 && myHqHp <= max(5, R.hqHp[hqLvl] / 2);


    D.hqEmergency = D.foeAtHQ > 0 || D.foeNearHQ >= 3 || D.massRush ||
                    (hqLow && D.foeNearHQ > 0);
    if (day >= 170 && hqLow && D.foeMyHalf > 0) D.hqEmergency = true;


    D.besieged = D.hqEmergency || D.hitBase >= 0 || D.foeMyHalf >= 3;


    if (D.hqEmergency)        D.defendCell = W.capMine;
    else if (D.hitBase >= 0)  D.defendCell = D.hitBase;
    else if (D.clusterCell >= 0 && D.foeMyHalf >= 3) {

        int best = W.capMine; double bd = W.reachMine[D.clusterCell];
        for (auto& b : W.base) {
            if (b.second.flag != W.me) continue;
            int z = b.first;
            if (W.hereFoe(z) > 0) continue;
            double d = fabs(W.reachMine[z] - W.reachMine[D.clusterCell]);
            if (d < bd) { bd = d; best = z; }
        }
        D.defendCell = best;
    }


    int turret = 0;
    if (W.base.count(D.defendCell)) turret = W.citadelGun(W.base[D.defendCell]);
    if (D.hqEmergency) {
        D.guard = max({4, D.foeAtHQ + 4, D.foeNearHQ + 2, D.clusterN + 1 - turret});
        if (D.massRush) D.guard = max(D.guard, D.clusterN + 4);
    } else if (D.hitBase >= 0) {
        D.guard = max(3, W.hereFoe(D.hitBase) + 3 - turret);
    } else if (D.foeMyHalf >= 3) {
        D.guard = max(3, D.clusterN + 2 - turret);
    } else {
        D.guard = max(0, D.fieldFoe - R.hqGun[hqLvl]);
    }
    D.guard = max(0, D.guard);


    int popCap = D.hqEmergency ? W.popMine() : (W.popMine() * 7) / 10;
    D.guard = min(D.guard, max(2, popCap));
    return D;
}


static void phaseBuild(long long& budget, set<int>& done, int& fortCnt, const Danger& D, int& hqLvl) {
    int goal = fortTarget();


    for (auto& s : W.army) {
        if (W.purse < R.fcBuild[1]) break;
        if (s.second.flag != W.me || s.second.walking) continue;
        int z = s.second.spot;
        if (z == W.capMine || !W.fortable[z] || W.base.count(z) || done.count(z)) continue;
        if (W.hereFoe(z) > 0 || !W.inMyReach(z)) continue;
        cout << "UPGRADE " << z << "\n";
        W.purse -= R.fcBuild[1]; budget -= R.fcBuild[1]; done.insert(z);
        W.base[z] = {W.me, 1, R.fcHp[1], false}; fortCnt++;
    }

    bool capitalClear = (W.hereFoe(W.capMine) == 0 && !done.count(W.capMine));
    bool capitalManned = capitalClear && W.hereMine(W.capMine) >= 1;


    if (capitalClear && W.hereMine(W.capMine) >= 1) {
        int maxHp = R.hqHp[hqLvl];
        if (W.base[W.capMine].hp <= maxHp / 2 && budget >= R.hqRepair) {
            cout << "UPGRADE " << W.capMine << "\n";
            W.purse -= R.hqRepair; budget -= R.hqRepair; done.insert(W.capMine);
            W.base[W.capMine].hp = maxHp;
            capitalManned = false;
        }
    }


    if (capitalManned && !D.besieged) {
        if (hqLvl < CFG.hqTop && fortCnt >= 1) {
            long long bill = R.hqUpgrade[hqLvl + 1];

            long long pad = (hqLvl <= 2) ? 150
                          : (fortCnt >= goal) ? 100
                          : (fortCnt >= goal - 1 ? 300 : 500);
            if (budget >= bill + pad) {
                cout << "UPGRADE " << W.capMine << "\n";
                W.purse -= bill; budget -= bill; done.insert(W.capMine);
                W.base[W.capMine].lvl = ++hqLvl;
                W.base[W.capMine].hp  = R.hqHp[hqLvl];
                capitalManned = false;
            }
        } else if (hqLvl >= CFG.hqTop &&
                   W.base[W.capMine].hp < R.hqHp[hqLvl] && budget >= R.hqRepair) {

            cout << "UPGRADE " << W.capMine << "\n";
            W.purse -= R.hqRepair; budget -= R.hqRepair; done.insert(W.capMine);
            W.base[W.capMine].hp = R.hqHp[hqLvl];
        }
    }


    if (!D.besieged) {
        long long hqReserve = (hqLvl < CFG.hqTop) ? R.hqUpgrade[hqLvl + 1] : 0;
        for (auto& b : W.base) {
            Citadel& c = b.second; int z = b.first;
            if (c.flag != W.me || c.capital || c.lvl >= CFG.fortTop) continue;
            if (done.count(z) || W.hereFoe(z) > 0 || W.hereMine(z) < W.citadelJob(c)) continue;
            long long bill = R.fcBuild[c.lvl + 1];
            long long pad = (fortCnt >= goal) ? 150 : 500;

            if (budget - hqReserve >= bill + pad) {
                cout << "UPGRADE " << z << "\n";
                W.purse -= bill; budget -= bill; done.insert(z);
                c.lvl++; c.hp = R.fcHp[c.lvl];
            }
        }
    }


    if (!D.besieged && budget > 1500) {
        for (auto& b : W.base) {
            Citadel& c = b.second; int z = b.first;
            if (c.flag != W.me || c.capital) continue;
            if (done.count(z) || W.hereFoe(z) > 0 || W.hereMine(z) < 1) continue;
            if (c.hp >= R.fcHp[c.lvl]) continue;
            if (budget < R.fcRepair + 1000) break;
            cout << "UPGRADE " << z << "\n";
            W.purse -= R.fcRepair; budget -= R.fcRepair; done.insert(z);
            c.hp = R.fcHp[c.lvl];
        }
    }
}


static void phaseMint(long long& budget, int fortCnt, const Danger& D, int hqLvl) {


    long long earmark = 0;
    if (fortCnt < fortTarget()) {

        int need = fortTarget() - fortCnt;
        earmark = 300LL * need;
    }
    if (D.besieged) earmark = 0;


    int movers = 0;
    for (auto& s : W.army)
        if (s.second.flag == W.me && !s.second.walking) movers++;
    long long moveReserve = 10LL * movers;
    earmark += moveReserve;

    int limit = R.hqMint[hqLvl];
    long long free = budget - earmark;


    int make = 0;
    if (free >= 120) make = min(limit, (int)(free / 120));


    while (make > 0 && 120LL * make > W.purse) make--;

    if (make > 0) {
        cout << "TRAIN " << make << "\n";
        W.purse -= 120LL * make; budget -= 120LL * make;
    }
}


static void phaseMove(long long& budget, const Danger& D, int hqLvl, int day) {
    (void)day;

    int cap    = W.capMine;
    int capFoe = W.capFoe;
    int f1 = W.fort1, f2 = W.fort2;


    auto ownFort = [&](int z) {
        return z >= 0 && W.base.count(z) && W.base[z].flag == W.me && !W.base[z].capital;
    };


    auto pickTarget = [&]() -> int {
        int best = -1; double bestD = 1e18;
        for (auto& b : W.base) {
            if (b.second.flag != W.foe || b.second.capital) continue;
            int z = b.first;
            if (W.reachMine[z] >= 1e17) continue;
            double d = W.reachMine[z];
            if (d < bestD) { bestD = d; best = z; }
        }
        if (best >= 0) return best;
        return capFoe;
    };
    int atkTarget = pickTarget();


    auto isFoeFort = [&](int z) {
        return z >= 0 && W.base.count(z) && W.base[z].flag == W.foe && !W.base[z].capital;
    };

    int engaged = -1; double engD = 1e18;
    for (auto& b : W.base) {
        if (b.second.flag != W.foe || b.second.capital) continue;
        int z = b.first;
        if (W.hereMine(z) > 0 && W.reachMine[z] < engD) { engD = W.reachMine[z]; engaged = z; }
    }
    if (engaged >= 0) {
        W.lockTarget = engaged;
        atkTarget = engaged;
    } else {


        atkTarget = pickTarget();
        W.lockTarget = isFoeFort(atkTarget) ? atkTarget : -1;
    }


    {
        int rally0 = ownFort(f2) ? f2 : (ownFort(f1) ? f1 : cap);

        if (atkTarget >= 0 && atkTarget < W.cells && rally0 != atkTarget) {
            vector<double> dist(W.cells, 1e18);
            vector<int> par(W.cells, -1);
            priority_queue<pair<double,int>, vector<pair<double,int>>, greater<>> pq;
            dist[rally0] = 0; pq.push({0, rally0});
            while (!pq.empty()) {
                auto [d, u] = pq.top(); pq.pop();
                if (d > dist[u] + 1e-9) continue;
                for (int v : W.nbr[u]) {
                    double nd = d + edgeLen(u, v);
                    if (nd < dist[v]) { dist[v] = nd; par[v] = u; pq.push({nd, v}); }
                }
            }


            if (dist[atkTarget] < 1e17) {
                vector<int> path;
                for (int c = atkTarget; c != -1; c = par[c]) {
                    path.push_back(c);
                    if (c == rally0) break;
                }

                for (int i = (int)path.size() - 1; i >= 0; --i) {
                    int z = path[i];
                    if (z == rally0) continue;
                    if (isFoeFort(z)) { atkTarget = z; W.lockTarget = z; break; }
                    if (z == atkTarget) break;
                }
            }
        }
    }


    int hqKeep = max(1, R.hqJob[hqLvl]);
    if (D.besieged && D.defendCell == cap) hqKeep = max(hqKeep, D.guard);
    if (D.hqEmergency) hqKeep = max(hqKeep, D.guard);


    map<int,int> stay;
    vector<string> idle;
    for (auto& s : W.army) {
        if (s.second.flag != W.me || s.second.walking) continue;
        int z = s.second.spot;

        if (z == cap) {

            if (stay[z] < hqKeep) { stay[z]++; continue; }
            idle.push_back(s.first); continue;
        }
        if (z == f1 && ownFort(f1)) {

            int keep = 1;
            if (D.besieged && D.defendCell == f1) keep = max(keep, D.guard);
            if (stay[z] < keep) { stay[z]++; continue; }
            idle.push_back(s.first); continue;
        }
        if (z == f2 && ownFort(f2)) {

            int keep = 1;
            if (D.besieged && D.defendCell == f2) keep = max(keep, D.guard);
            if (stay[z] < keep) { stay[z]++; continue; }
            idle.push_back(s.first); continue;
        }


        if (z == f2 && !ownFort(f2) && f2 >= 0 && W.hereFoe(z) == 0) {
            if (stay[z] < 1) { stay[z]++; continue; }
            idle.push_back(s.first); continue;
        }

        if (z == f1 && !ownFort(f1) && f1 >= 0 && W.hereFoe(z) == 0) {
            if (stay[z] < 1) { stay[z]++; continue; }
            idle.push_back(s.first); continue;
        }

        idle.push_back(s.first);
    }


    sort(idle.begin(), idle.end(), [&](const string& a, const string& b) {
        return W.reachMine[W.army[a].spot] < W.reachMine[W.army[b].spot];
    });
    size_t k = 0;


    if (!ownFort(f1) && f1 >= 0) {

        if (W.boundFor(f1) < 1 && k < idle.size()) {
            order(idle[k++], f1, budget);
        }
    }
    if (!ownFort(f2) && f2 >= 0 && f2 != f1) {
        if (W.boundFor(f2) < 1 && k < idle.size()) {
            order(idle[k++], f2, budget);
        }
    }


    if (D.besieged) {
        int dz = D.defendCell;
        int have = W.boundFor(dz);

        while (k < idle.size() && have < D.guard) {
            if (W.army[idle[k]].spot == dz) { k++; continue; }
            order(idle[k++], dz, budget); have++;
        }

        if (have < D.guard && D.hqEmergency) {
            vector<string> back;
            for (auto& s : W.army) {
                if (s.second.flag != W.me || s.second.walking) continue;
                int z = s.second.spot;
                if (z == dz) continue;
                back.push_back(s.first);
            }
            sort(back.begin(), back.end(), [&](const string& a, const string& b){
                return W.reachMine[W.army[a].spot] < W.reachMine[W.army[b].spot];
            });
            for (auto& id : back) {
                if (have >= D.guard) break;
                order(id, dz, budget); have++;
            }
        }
    }


    int rally = ownFort(f2) ? f2 : (ownFort(f1) ? f1 : cap);


    int atRally = W.hereMine(rally);


    for (size_t i = k; i < idle.size(); ++i) {
        const string& id = idle[i];
        int z = W.army[id].spot;
        if (W.army[id].walking) continue;

        if (z == cap || z == f1 || z == f2) continue;

        order(id, atkTarget, budget);
    }


    const int LAUNCH = 6;
    if (ownFort(rally) && atRally >= LAUNCH) {
        int sendN = atRally - 1;
        vector<string> here;
        for (auto& s : W.army) {
            if (s.second.flag != W.me || s.second.walking) continue;
            if (s.second.spot == rally) here.push_back(s.first);
        }

        for (size_t i = 0; i < here.size() && sendN > 0; ++i) {
            order(here[i], atkTarget, budget); sendN--;
        }
    }


    while (k < idle.size()) {
        const string& id = idle[k++];
        int z = W.army[id].spot;
        if (z == rally) continue;

        if (z != cap && z != f1 && z != f2) continue;
        order(id, rally, budget);
    }
}


static void playTurn(int day) {
    cout << "COMMAND\n";

    if (!W.base.count(W.capMine)) { cout << "END\n" << flush; return; }


    long long budget = W.purse - CFG.safety;
    int hqLvl = W.base[W.capMine].lvl;


    Danger D = assessDanger(hqLvl, day);


    set<int> done;
    int fortCnt = 0;
    for (auto& b : W.base) if (b.second.flag == W.me && !b.second.capital) fortCnt++;


    phaseBuild(budget, done, fortCnt, D, hqLvl);
    phaseMint (budget, fortCnt, D, hqLvl);
    phaseMove (budget, D, hqLvl, day);

    cout << "END\n" << flush;
}


static void ingestResult() {
    int day; cin >> day;
    string tok; cin >> tok;
    long long a, b, c, d; cin >> a >> b >> c >> d;
    W.grace = (int)b;

    vector<pair<char,int>>            raised;
    vector<string>                   minted;
    vector<pair<string,int>>         stepped;
    vector<tuple<string,string,int>> harmed;
    vector<tuple<char,int,int>>      battered;

    while (cin >> tok) {
        if (tok == "END") break;
        int n; cin >> n;
        if (tok == "UPGRADE") {
            for (int i = 0; i < n; i++) { string s; int z; cin >> s >> z; raised.push_back({s[0], z}); }
        } else if (tok == "TRAIN") {
            for (int i = 0; i < n; i++) { string id; cin >> id; minted.push_back(id); }
        } else if (tok == "MOVE") {
            for (int i = 0; i < n; i++) { string id; int z; cin >> id >> z; stepped.push_back({id, z}); }
        } else if (tok == "DAMAGE") {
            for (int i = 0; i < n; i++) { string k, id; int h; cin >> k >> id >> h; harmed.push_back({k, id, h}); }
        } else if (tok == "SIEGE") {
            for (int i = 0; i < n; i++) { string s; int z, h; cin >> s >> z >> h; battered.push_back({s[0], z, h}); }
        } else {
            for (int i = 0; i < n; i++) { string junk; getline(cin, junk); }
        }
    }


    for (auto& [f, z] : raised) {
        if (f == W.me) {
            if (!W.base.count(z)) W.base[z] = {W.me, 1, R.fcHp[1], false};
            continue;
        }
        if (!W.base.count(z)) W.base[z] = {f, 1, R.fcHp[1], false};
        else {
            Citadel& cc = W.base[z];
            int top = cc.capital ? 5 : 3;
            if (cc.lvl < top) cc.lvl++;
            cc.hp = W.citadelHp(cc);
        }
    }

    for (auto& id : minted) {
        char f = id[0];
        W.army[id] = {f, W.capitalCell(f), R.hqUnitHp[W.capitalLvl(f)], false, -1};
    }

    for (auto& [id, z] : stepped) {
        auto it = W.army.find(id);
        if (it == W.army.end()) continue;
        it->second.spot = z;
        if (it->second.walking && it->second.spot == it->second.goal) it->second.walking = false;
    }

    for (auto& [k, id, h] : harmed) {
        if (k == "HUNGER") continue;
        auto it = W.army.find(id);
        if (it != W.army.end()) it->second.hp -= h;
    }
    for (auto& [f, z, h] : battered) {
        auto it = W.base.find(z);
        if (it != W.base.end()) it->second.hp -= h;
    }
    for (auto it = W.army.begin(); it != W.army.end();)
        it = (it->second.hp <= 0) ? W.army.erase(it) : next(it);
    for (auto it = W.base.begin(); it != W.base.end();)
        it = (it->second.hp <= 0) ? W.base.erase(it) : next(it);

    long long take = 0;
    for (auto& bb : W.base) {
        if (bb.second.flag != W.me) continue;
        take += 15LL * min(W.hereMine(bb.first), W.citadelJob(bb.second));
    }
    W.purse += take;

    W.purse -= 2LL * W.popMine();
    if (W.purse < 0) W.purse = 0;

    for (auto& [k, id, h] : harmed) {
        if (k != "HUNGER") continue;
        auto it = W.army.find(id);
        if (it != W.army.end()) it->second.hp -= h;
    }
    for (auto it = W.army.begin(); it != W.army.end();)
        it = (it->second.hp <= 0) ? W.army.erase(it) : next(it);
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string head;
    while (cin >> head) {
        if      (head == "READY")  bootstrap();
        else if (head == "START")  { string w; int d; cin >> w >> d; playTurn(d); }
        else if (head == "TURN")   ingestResult();
        else if (head == "FINISH") break;
    }
    return 0;
}
