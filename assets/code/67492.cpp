#include <bits/stdc++.h>
using namespace std;


static const int HQ_UP[6] = { 0, 0, 600, 1200, 2400, 3600 };
static const int HQ_REPAIR = 1000;
static const int HQ_WHP[6] = { 0, 4, 5, 6, 7, 8 };
static const int HQ_HP[6] = { 0, 10, 15, 20, 25, 30 };
static const int HQ_TUR[6] = { 0, 1, 2, 2, 3, 3 };
static const int HQ_TRAIN[6] = { 0, 1, 1, 2, 2, 3 };
static const int HQ_LAB[6] = { 0, 1, 2, 3, 4, 5 };
static const int BS_COST[4] = { 0, 300, 600, 1000 };
static const int BS_REPAIR = 500;
static const int BS_HP[4] = { 0, 6, 12, 18 };
static const int BS_TUR[4] = { 0, 1, 1, 2 };
static const int BS_LAB[4] = { 0, 1, 2, 3 };

static const long long GOLD_BUFFER = 120;
static const double EXPAND_REACH = 1.30;


int N, K;
vector<long long> PX, PY;
vector<char> isBase;
vector<vector<int>> adj;
vector<vector<double>> dist;
vector<vector<int>> hopDist;
vector<int> strongOrder;
vector<int> hopHQ;
int baseCap = 0;
int centerZone = -1;
int centerBase = -1;

char meC, oppC;
int myHQ, oppHQ;


long long gold = 500;
int byoyomi = 5;

struct War { char owner; int zone; int hp; bool moving; int target; };
map<string, War> war;
struct Bld { char owner; int level; int hp; bool hq; };
map<int, Bld> bld;
map<int, int> razeCnt;
map<int, int> buildDay;
int prevRallyZone = -1;
int prevAttackTgt = -1;
map<int, int> enemyRazeWatchUntil;
map<string, int> lastEnemyZone;
map<string, int> attackCommitTarget;

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
    bld[0] = { 'A', 1, HQ_HP[1], true };
    bld[N - 1] = { 'B', 1, HQ_HP[1], true };
    for (int i = 1; i <= 3; i++) {
        war[string("A") + to_string(i)] = { 'A', 0,     4, false, -1 };
        war[string("B") + to_string(i)] = { 'B', N - 1, 4, false, -1 };
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


    hopDist.assign(N, vector<int>(N, 1000000000));
    {
        vector<int> order(N);
        for (int s = 0; s < N; s++) {
            iota(order.begin(), order.end(), 0);
            sort(order.begin(), order.end(), [&](int a, int b) { return dist[s][a] < dist[s][b]; });
            hopDist[s][s] = 0;
            for (int v : order) {
                if (v == s || dist[s][v] >= 1e17) continue;
                int best = 1000000000;
                for (int u : adj[v]) {
                    double dx = (double)(PX[u] - PX[v]), dy = (double)(PY[u] - PY[v]);
                    double w = ceil(sqrt(dx * dx + dy * dy));
                    if (fabs(dist[s][u] + w - dist[s][v]) < 1e-6 && hopDist[s][u] < 1000000000)
                        best = min(best, hopDist[s][u] + 1);
                }
                hopDist[s][v] = best;
            }
        }
    }
    hopHQ = hopDist[myHQ];
    centerZone = 0;
    for (int z = 1; z < N; z++)
        if (llabs(PX[z]) + llabs(PY[z]) < llabs(PX[centerZone]) + llabs(PY[centerZone]))
            centerZone = z;
    for (int z = 0; z < N; z++) if (isBase[z]) strongOrder.push_back(z);
    sort(strongOrder.begin(), strongOrder.end(), [&](int a, int b) {
        if (hopHQ[a] != hopHQ[b]) return hopHQ[a] < hopHQ[b];
        if (fabs(dist[myHQ][a] - dist[myHQ][b]) > 1e-7) return dist[myHQ][a] < dist[myHQ][b];
        return a < b;
        });


    baseCap = 0;


    double reachCap = (K <= 9) ? 1.0 : EXPAND_REACH;
    for (int z : strongOrder) if (dist[myHQ][z] <= dist[oppHQ][z] * reachCap) baseCap++;


    centerBase = -1;
    {
        long long bestc = LLONG_MAX;
        for (int z : strongOrder) {
            if (z == myHQ || z == oppHQ) continue;
            if (dist[myHQ][z] > dist[oppHQ][z]) continue;
            long long c = llabs(PX[z]) + llabs(PY[z]);
            if (c < bestc) { bestc = c; centerBase = z; }
        }
    }

    gold = 500; byoyomi = 5;
    cout << "OK\n" << flush;
}


void doCommands(int day) {
    cout << "COMMAND\n";
    if (!bld.count(myHQ)) { cout << "END\n" << flush; return; }
    long long avail = gold - GOLD_BUFFER;
    int hl = bld[myHQ].level;
    const int cmdHqLevel = hl;
    set<int> friendlyBldAtTurnStart;
    for (auto& kv : bld) if (kv.second.owner == meC) friendlyBldAtTurnStart.insert(kv.first);
    set<string> movedIssuedThisTurn;


    int eField = 0, eActiveField = 0, eArmyTotal = 0, spearHop = INT_MAX, spearZone = -1;
    int allFieldSpearHop = INT_MAX, allFieldSpearZone = -1;
    vector<int> enemyFieldCnt(N, 0), enemyActiveFieldCnt(N, 0);
    map<int, int> onEnemyBld;
    for (auto& kv : war) {
        if (kv.second.owner != oppC) continue;
        int z = kv.second.zone;
        if (z < 0 || z >= N) continue;
        eArmyTotal++;
        bool worker = bld.count(z) && bld[z].owner == oppC;
        if (worker) { onEnemyBld[z]++; continue; }
        eField++;
        enemyFieldCnt[z]++;
        bool nearEnemyBuilding = false;
        for (auto& eb : bld) {
            if (eb.second.owner != oppC) continue;
            if (hopDist[z][eb.first] <= 1) { nearEnemyBuilding = true; break; }
        }
        bool crossedEnemyBoundary = !nearEnemyBuilding || dist[myHQ][z] <= dist[oppHQ][z];
        if (!crossedEnemyBoundary) continue;
        eActiveField++;
        enemyActiveFieldCnt[z]++;
        if (hopHQ[z] < spearHop) { spearHop = hopHQ[z]; spearZone = z; }
    }
    int myArmy = myCount();
    bool dominate = (myArmy >= eArmyTotal + 4);
    int hqTur = HQ_TUR[hl];
    int hqHopWidth = (oppHQ >= 0 && oppHQ < (int)hopHQ.size()) ? hopHQ[oppHQ] : 1000000000;


    int naturalHold = hqTur + HQ_LAB[hl];
    bool menace = false;
    if (eActiveField > naturalHold && spearZone >= 0) {
        if (dist[myHQ][spearZone] <= dist[oppHQ][spearZone]) menace = true;
        else if (hqHopWidth < 1000000000 && spearHop * 10 <= hqHopWidth * 6) menace = true;
    }


    int eStagedSurplus = 0;
    for (auto& kv : onEnemyBld) {
        auto ib = bld.find(kv.first);
        if (ib != bld.end() && ib->second.owner == oppC)
            eStagedSurplus += max(0, kv.second - bldLabCap(ib->second));
    }
    int threatArmy = eField + eStagedSurplus;
    int launchEta = 1000000000;
    for (auto& kv : war) {
        if (kv.second.owner != oppC) continue;
        int ez = kv.second.zone; if (ez < 0 || ez >= N) continue;
        for (auto& b : bld)
            if (b.second.owner == meC && hopDist[ez][b.first] < launchEta) launchEta = hopDist[ez][b.first];
    }
    int mobileArmy = myArmy - min(myArmy, totalWorkCap());
    bool armyBehind = threatArmy >= 3 && launchEta < 1000000000 &&
        mobileArmy + HQ_TRAIN[hl] * launchEta < threatArmy + 1;
    int preAlertHop = (hqHopWidth < 1000000000) ? max(4, (hqHopWidth * 7 + 9) / 10) : 6;
    bool gatheringPressure = (allFieldSpearZone >= 0 && allFieldSpearHop <= preAlertHop && eField >= max(4, naturalHold + 1));
    bool stagingPressure = (threatArmy >= max(4, naturalHold + 2) && launchEta <= preAlertHop);
    bool hqPressure = (oppInZone(myHQ) > 0) || (spearZone >= 0 && spearHop <= 2 && eActiveField >= max(2, naturalHold)) || gatheringPressure || stagingPressure;
    bool defenseModeCore = menace || armyBehind || hqPressure || eActiveField >= max(5, naturalHold + 2);

    set<int> usedUpg;

    int nBases = 0;
    for (auto& kv : bld) if (kv.second.owner == meC && !kv.second.hq) nBases++;
    bool expandDone = (nBases >= baseCap);


    bool majorityBases = (nBases * 2 > K);
    double expandReach = (majorityBases || K <= 9) ? 1.0 : EXPAND_REACH;
    bool econOK = expandDone || majorityBases || nBases >= 4;


    int oppBases = 0;
    for (auto& kv : bld) if (kv.second.owner == oppC && !kv.second.hq) oppBases++;
    bool earlyRush = (nBases < 2 && day <= 40 && oppBases == 0 && eArmyTotal >= 4);
    defenseModeCore = defenseModeCore || earlyRush;


    auto staffOf = [&](int z) {
        int c = 0;
        for (auto& kv : war)
            if (kv.second.owner == meC &&
                (kv.second.zone == z || (kv.second.moving && kv.second.target == z))) c++;
        return c;
        };
    auto canBuildSite = [&](int z) {

        return dist[myHQ][z] <= dist[oppHQ][z];
        };
    bool safeExpansionLeft = false;
    for (int z : strongOrder) {
        if (bld.count(z) || staffOf(z) > 0) continue;
        if (!canBuildSite(z)) continue;
        if (razeCnt[z] >= 2 && !dominate) continue;
        if (oppInZone(z) > 0) continue;
        safeExpansionLeft = true;
        break;
    }

    auto adjEnemyField = [&](int z) {
        int c = 0;
        for (int a : adj[z]) {
            if (bld.count(a) && bld[a].owner == oppC) continue;
            c += oppInZone(a);
        }
        return c;
        };


    auto pathUnsafe = [&](int from, int to) {
        if (from == to) return false;
        double D = dist[from][to];
        if (D >= 1e17) return true;
        for (auto& kv : bld) {
            if (kv.second.owner != oppC) continue;
            int z = kv.first;
            if (z == to || z == from) continue;
            if (dist[from][z] + dist[z][to] <= D + 1e-6) return true;
        }
        for (int z = 0; z < N; z++) {
            if (z == to || z == from || enemyFieldCnt[z] < 2) continue;
            if (dist[from][z] + dist[z][to] <= D + 1e-6) return true;
        }
        return false;
        };
    for (auto it = enemyRazeWatchUntil.begin(); it != enemyRazeWatchUntil.end();) {
        if (it->second < day || bld.count(it->first)) it = enemyRazeWatchUntil.erase(it);
        else ++it;
    }
    auto shouldWatchRazedEnemyBase = [&](int z) {
        if (!isBase[z] || bld.count(z)) return false;
        auto it = enemyRazeWatchUntil.find(z);
        if (it == enemyRazeWatchUntil.end() || it->second < day) return false;
        if (oppInZone(z) > 0 || adjEnemyField(z) >= 2) return false;
        int nearEnemy = 0;
        for (int a : adj[z]) {
            nearEnemy += enemyFieldCnt[a];
            for (int b : adj[a]) if (b != z) nearEnemy += enemyFieldCnt[b];
        }
        return nearEnemy < 3;
        };

    int mainThreatZone = -1, mainThreatCnt = 0, mainThreatEta = 1000000000;
    for (int z = 0; z < N; z++) {
        int e = enemyActiveFieldCnt[z];
        if (e <= 0) continue;
        int eta = hopDist[z][myHQ];
        if (mainThreatZone < 0 || e > mainThreatCnt || (e == mainThreatCnt && eta < mainThreatEta)) {
            mainThreatZone = z; mainThreatCnt = e; mainThreatEta = eta;
        }
    }


    int predTarget = -1, predCnt = 0, predEta = 1000000000;
    {
        map<int, int> appCnt, appEta;
        for (auto& kv : war) {
            if (kv.second.owner != oppC) continue;
            int z = kv.second.zone; if (z < 0 || z >= N) continue;
            if (bld.count(z) && bld[z].owner == oppC) continue;
            auto it = lastEnemyZone.find(kv.first);
            if (it == lastEnemyZone.end()) continue;
            int pz = it->second;
            if (pz < 0 || pz >= N || pz == z) continue;
            for (auto& b : bld) {
                if (b.second.owner != meC) continue;
                int bz = b.first;
                if (hopDist[z][bz] >= 1000000000 || hopDist[pz][bz] >= 1000000000) continue;
                if (hopDist[z][bz] < hopDist[pz][bz]) {
                    appCnt[bz]++;
                    auto ie = appEta.find(bz);
                    if (ie == appEta.end() || hopDist[z][bz] < ie->second) appEta[bz] = hopDist[z][bz];
                }
            }
        }
        for (auto& pr : appCnt) {
            int eta = appEta.count(pr.first) ? appEta[pr.first] : 1000000000;
            if (pr.second > predCnt || (pr.second == predCnt && eta < predEta)) {
                predTarget = pr.first; predCnt = pr.second; predEta = eta;
            }
        }
        if (predCnt < 3) { predTarget = -1; predCnt = 0; }
    }


    auto currentIncome = [&]() {
        long long s = 0;
        for (auto& kv : bld)
            if (kv.second.owner == meC)
                s += 15LL * min(myInZone(kv.first), bldLabCap(kv.second));
        return s;
        };
    auto multiValueOf = [&](char owner) {
        int v = 0;
        for (auto& kv : bld)
            if (kv.second.owner == owner && !kv.second.hq)
                v += kv.second.level * 2 + bldLabCap(kv.second);
        return v;
        };
    int myMultiValue = multiValueOf(meC), oppMultiValue = multiValueOf(oppC);
    auto hqStrategicValueOf = [&](char owner) {
        int h = hqLevelOf(owner);
        return HQ_LAB[h] * 2 + HQ_TRAIN[h] * 6 + HQ_WHP[h] + HQ_TUR[h] * 2 + HQ_HP[h] / 5;
        };
    int myHqStrategicValue = hqStrategicValueOf(meC), oppHqStrategicValue = hqStrategicValueOf(oppC);
    int myStrategicValue = myMultiValue + myHqStrategicValue;
    int oppStrategicValue = oppMultiValue + oppHqStrategicValue;
    bool econBehind = (myMultiValue + HQ_LAB[hl] * 2 < oppMultiValue + HQ_LAB[hqLevelOf(oppC)] * 2);
    bool strategicBehind = (myStrategicValue < oppStrategicValue);

    int idleMy = 0;
    for (auto& kv : war) if (kv.second.owner == meC && !kv.second.moving) idleMy++;
    long long combatMoveReserve = 0;
    if (earlyRush || menace || eField > 0)
        combatMoveReserve = 10LL * min(idleMy, max(3, eField + 2));
    if (day >= 120)
        combatMoveReserve = max(combatMoveReserve, 10LL * min(idleMy, 10));
    avail -= combatMoveReserve;
    long long commandFloor = GOLD_BUFFER + (day >= 120 ? (defenseModeCore ? 120 : 220) : 40);
    long long commandGold = max(0LL, gold - commandFloor);
    commandGold = min(commandGold, max(0LL, avail));
    avail = commandGold;


    bool techWorthNow = (hl < 5) && (HQ_UP[hl + 1] <= currentIncome() * 15);
    bool lowHqTechPriority = (hl < 3 && !safeExpansionLeft && !menace && !armyBehind && eActiveField < max(5, naturalHold + 2) && nBases >= 2 && techWorthNow);
    const long long HQ_WA_MARGIN = 1200;

    auto ownReadyBuilderAt = [&](int z) {
        for (auto& kv : war)
            if (kv.second.owner == meC && !kv.second.moving && kv.second.zone == z)
                return true;
        return false;
        };

    auto engineUpgradeCost = [&](int z) -> long long {
        auto ib = bld.find(z);
        if (ib == bld.end() || ib->second.owner != meC) return 300;
        if (ib->second.hq) return ib->second.level < 5 ? (long long)HQ_UP[ib->second.level + 1] : (long long)HQ_REPAIR;
        return ib->second.level < 3 ? (long long)BS_COST[ib->second.level + 1] : (long long)BS_REPAIR;
        };
    auto issueUpgrade = [&](int z, long long cost) -> bool {
        if (z < 0 || z >= N || usedUpg.count(z) || commandGold < cost || gold < cost) return false;
        if (!ownReadyBuilderAt(z) || oppInZone(z) > 0) return false;
        auto ib = bld.find(z);
        if (ib != bld.end()) {
            if (ib->second.owner != meC) return false;
            if (ib->second.hq) {
                if (ib->second.level >= 5 && ib->second.hp >= HQ_HP[5]) return false;
            }
            else if (ib->second.level >= 3 && ib->second.hp >= BS_HP[3]) return false;
        }
        else if (!isBase[z]) return false;
        cost = engineUpgradeCost(z);
        if (commandGold < cost || gold < cost) return false;
        cout << "UPGRADE " << z << "\n";
        gold -= cost; avail -= cost; commandGold -= cost; usedUpg.insert(z);
        return true;
        };
    auto upkeepSafeGoldAfterTrain = [&](int trainCnt) {
        int afterUnits = myCount() + trainCnt;
        long long upkeep = 2LL * afterUnits;
        long long income = currentIncome();
        long long deficit = max(0LL, upkeep - income);
        return GOLD_BUFFER + upkeep + deficit * 2;
        };
    auto issueTrain = [&](int cnt, bool ignoreUpkeep = false) -> bool {
        cnt = min(cnt, HQ_TRAIN[cmdHqLevel]);
        while (cnt > 0 && commandGold < 120LL * cnt) cnt--;
        while (!ignoreUpkeep && cnt > 0 && gold - 120LL * cnt < upkeepSafeGoldAfterTrain(cnt)) cnt--;
        if (cnt <= 0 || gold < 120LL * cnt || commandGold < 120LL * cnt) return false;
        cout << "TRAIN " << cnt << "\n";
        gold -= 120LL * cnt; avail -= 120LL * cnt; commandGold -= 120LL * cnt;
        return true;
        };


    for (auto& kv : war) {
        if (gold < 300) break;
        if (earlyRush && nBases >= 1) break;
        if (armyBehind && nBases >= 2) break;
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        if (z == myHQ || !isBase[z] || bld.count(z) || usedUpg.count(z)) continue;
        if (oppInZone(z) > 0) continue;
        bool forwardBuildSite = day >= 145 && !menace && !armyBehind &&
            (majorityBases || myArmy >= eArmyTotal + 8 || oppMultiValue >= myMultiValue) &&
            dist[oppHQ][z] >= dist[myHQ][oppHQ] * 0.25 &&
            dist[myHQ][z] <= dist[oppHQ][z] * 1.45 &&
            200 - day > 24;
        if (!canBuildSite(z) && !forwardBuildSite) continue;


        if (razeCnt[z] >= 2 && !dominate) continue;


        if (majorityBases && techWorthNow && dist[myHQ][z] > dist[oppHQ][z] && !forwardBuildSite) continue;


        if (dist[oppHQ][z] < dist[myHQ][oppHQ] * 0.20) continue;
        if (!issueUpgrade(z, 300)) continue;
        bld[z] = { meC, 1, BS_HP[1], false }; nBases++; buildDay[z] = day;
    }


    int eHQlv = bld.count(oppHQ) ? bld[oppHQ].level : 1;


    int eHQhpNow = bld.count(oppHQ) ? bld[oppHQ].hp : HQ_HP[1];


    bool hqImminent = (oppInZone(myHQ) > 0) || (bld[myHQ].hp < HQ_HP[hl]) || (spearZone >= 0 && spearHop <= 1);
    bool endgameTieTech = (day >= 145 && hl < 5 && bld[myHQ].hp <= eHQhpNow && !hqImminent);

    auto wantTechLead = [&]() {
        return day >= 145 && hl <= eHQlv && hl < 5 && !menace && !econBehind &&
            (myMultiValue > oppMultiValue || majorityBases) &&
            (expandDone || majorityBases || nBases >= 4);
        };
    if (oppInZone(myHQ) == 0 && ownReadyBuilderAt(myHQ) && !usedUpg.count(myHQ)) {
        if (hl < 5 && !defenseModeCore) {
            long long need = HQ_UP[hl + 1];


            long long buffer = (hl < eHQlv) ? 80 : (wantTechLead() ? 80 : (econBehind ? (lowHqTechPriority ? 160 : 1200) : ((expandDone || majorityBases) ? 200 : ((hl < 3) ? 240 : 1500))));
            long long hqSafety = buffer + HQ_WA_MARGIN;
            if ((econOK || lowHqTechPriority) && avail >= need + hqSafety && gold >= need + HQ_WA_MARGIN) {
                if (issueUpgrade(myHQ, need)) {
                    bld[myHQ].level = ++hl; bld[myHQ].hp = HQ_HP[hl];
                }
            }
        }
        else if (hl >= 5 && bld[myHQ].hp <= HQ_HP[5] - 5 && avail >= HQ_REPAIR + HQ_WA_MARGIN && gold >= HQ_REPAIR + HQ_WA_MARGIN) {
            if (issueUpgrade(myHQ, HQ_REPAIR)) bld[myHQ].hp = HQ_HP[5];
        }
    }


    if (endgameTieTech && oppInZone(myHQ) == 0 && ownReadyBuilderAt(myHQ) && !usedUpg.count(myHQ)) {
        long long need = HQ_UP[hl + 1];
        if (issueUpgrade(myHQ, need)) { bld[myHQ].level = ++hl; bld[myHQ].hp = HQ_HP[hl]; }
    }


    for (auto& kv : bld) {
        if (defenseModeCore) break;
        Bld& b = kv.second; int z = kv.first;
        if (b.owner != meC || b.hq || b.level >= 2) continue;
        if (dist[myHQ][z] > dist[oppHQ][z]) continue;
        if (usedUpg.count(z) || oppInZone(z) > 0 || myInZone(z) < bldLabCap(b)) continue;
        long long c = BS_COST[b.level + 1];

        int extraWorkers = max(1, BS_LAB[b.level + 1] - BS_LAB[b.level]);
        int breakEvenTurns = (int)((c + 15LL * extraWorkers - 1) / (15LL * extraWorkers)) + 5;
        if (200 - day <= breakEvenTurns) continue;
        if (lowHqTechPriority || defenseModeCore) continue;
        long long buffer = econBehind ? 80 : ((expandDone || majorityBases) ? 200 : 800);
        if ((expandDone || econBehind || majorityBases) && avail >= c + buffer) {


            if (!issueUpgrade(z, c)) continue;
            b.level++; b.hp = BS_HP[b.level];
        }
    }


    {

        long long baseReserve = 0;
        for (auto& kv : war) {
            if (kv.second.owner != meC) continue;
            int tg = kv.second.moving ? kv.second.target : kv.second.zone;
            bool immediateBuildSpot = (!kv.second.moving && kv.second.zone == tg);
            if (tg >= 0 && tg < N && tg != myHQ && isBase[tg] &&
                canBuildSite(tg) &&
                (200 - day > 24 || immediateBuildSpot) &&
                !(bld.count(tg) && bld[tg].owner == meC) && oppInZone(tg) == 0)
                baseReserve += 300;
        }
        if (baseReserve > 900) baseReserve = 900;

        if (defenseModeCore) baseReserve = 0;
        int cap = HQ_TRAIN[cmdHqLevel];
        int twc = totalWorkCap();
        int units = myCount();
        int sustain = 6 * max(1, twc);


        bool hqReserveReady = ownReadyBuilderAt(myHQ) && oppInZone(myHQ) == 0 && !usedUpg.count(myHQ);
        long long hqReserve = (hqReserveReady && !defenseModeCore && (hl < eHQlv || wantTechLead() || lowHqTechPriority) && hl < 5) ? (HQ_UP[hl + 1] + HQ_WA_MARGIN) : 0;


        if (endgameTieTech && hqReserveReady) hqReserve = max(hqReserve, (long long)HQ_UP[hl + 1]);
        long long trainable = avail - baseReserve - hqReserve;
        int want = 0;


        if (trainable > 0 && (units < sustain || trainable > 1200)) want = min(cap, (int)(trainable / 120));
        if (defenseModeCore && trainable >= 120)
            want = max(want, min(cap, (int)(trainable / 120)));


        if ((nBases < 1 && !earlyRush) || (nBases < 2 && !earlyRush && day <= 25)) want = 0;


        int workerDeficit = 0;
        for (auto& kv : bld) {
            if (kv.second.owner != meC) continue;
            int z = kv.first;
            if (!kv.second.hq && dist[myHQ][z] > dist[oppHQ][z]) continue;
            if (oppInZone(z) > 0) continue;
            workerDeficit += max(0, bldLabCap(kv.second) - staffOf(z));
        }
        if (workerDeficit > 0 && trainable >= 120)
            want = max(want, min(cap, min(workerDeficit, (int)(trainable / 120))));


        int futureExpansionWorkers = 0;
        bool defenseMode = defenseModeCore;
        if (!defenseMode && nBases >= 1) {
            int plannedBases = 0;
            int trainedBefore = 0;
            for (int z : strongOrder) {
                if (bld.count(z) || staffOf(z) > 0) continue;
                if (razeCnt[z] >= 2 && !dominate) continue;
                if (!canBuildSite(z)) continue;
                if (dist[myHQ][z] > dist[oppHQ][z] * expandReach) continue;
                if (oppInZone(z) > 0) continue;
                int eta = (hopHQ[z] >= 1000000000) ? 0 : hopHQ[z] + 1;
                long long projected = gold
                    - 120LL * (trainedBefore + 1)
                    + currentIncome() * eta
                    - 2LL * (myCount() + trainedBefore + 1) * eta
                    - 300LL * plannedBases;
                if (projected >= 300) {
                    futureExpansionWorkers++;
                    trainedBefore++;
                    plannedBases++;
                    if (futureExpansionWorkers >= cap) break;
                }
            }
        }
        if (futureExpansionWorkers > 0 && trainable >= 120)
            want = max(want, min(cap, min(workerDeficit + futureExpansionWorkers, (int)(trainable / 120))));


        long long warGold = gold - combatMoveReserve;
        long long payCap = avail;
        bool emergencyTrain = (units <= 1) || (earlyRush && day <= 40);
        if (emergencyTrain && warGold >= 120) {
            want = max(want, min(cap, (int)(warGold / 120)));
            payCap = max(payCap, warGold);
        }

        while (want > 0 && 120LL * want > payCap) want--;
        issueTrain(want, emergencyTrain);
    }


    auto committedTargetLive = [&](const string& w) -> int {
        auto ic = attackCommitTarget.find(w);
        if (ic == attackCommitTarget.end()) return -1;
        auto ib = bld.find(ic->second);
        if (ib == bld.end() || ib->second.owner != oppC) {
            attackCommitTarget.erase(ic);
            return -1;
        }
        return ib->first;
        };
    auto sendTo = [&](const string& w, int dest) -> bool {
        auto it = war.find(w);
        if (it == war.end() || it->second.owner != meC) return false;
        if (dest < 0 || dest >= N) return false;
        int committed = committedTargetLive(w);
        if (committed >= 0 && dest != committed) return false;
        if (it->second.moving || movedIssuedThisTurn.count(w)) return false;
        if (it->second.zone == dest) return true;
        long long mc = friendlyBldAtTurnStart.count(dest) ? 0 : 10;
        if (gold < mc || commandGold < mc) return false;
        cout << "MOVE " << w << " " << dest << "\n";
        gold -= mc; avail -= mc; commandGold -= mc;
        it->second.moving = true; it->second.target = dest;
        movedIssuedThisTurn.insert(w);
        auto ib = bld.find(dest);
        if (ib != bld.end() && ib->second.owner == oppC) attackCommitTarget[w] = dest;
        return true;
        };


    bool bigEnemyField = (eActiveField >= max(5, naturalHold + 2));

    int rallyZone = myHQ; {
        double rb = 1e18, prevAvg = 1e18;
        for (auto& kv : bld) {
            if (kv.second.owner != meC) continue;
            double sum = 0; int cnt = 0;
            for (auto& eb : bld) if (eb.second.owner == oppC && dist[kv.first][eb.first] < 1e17) { sum += dist[kv.first][eb.first]; cnt++; }
            double avg = cnt ? sum / cnt : dist[kv.first][oppHQ];
            if (kv.first == prevRallyZone) prevAvg = avg;
            if (avg < rb) { rb = avg; rallyZone = kv.first; }
        }


        if (prevAvg < 1e17 && prevAvg <= rb * 1.15) rallyZone = prevRallyZone;
    }

    int pathMultiZone = -1;
    if (menace && mainThreatZone >= 0 && mainThreatEta < 1000000000) {
        int bestEnemyArr = 1000000000, bestBack = -1;
        for (auto& kv : bld) {
            if (kv.second.owner != meC || kv.second.hq) continue;
            int z = kv.first;
            if (hopDist[mainThreatZone][z] >= 1000000000 || hopDist[z][myHQ] >= 1000000000) continue;
            if (hopDist[mainThreatZone][z] + hopDist[z][myHQ] != mainThreatEta) continue;
            int enemyArr = hopDist[mainThreatZone][z];
            int back = hopDist[z][myHQ];
            if (enemyArr < bestEnemyArr || (enemyArr == bestEnemyArr && back > bestBack)) {
                pathMultiZone = z; bestEnemyArr = enemyArr; bestBack = back;
            }
        }
    }

    if (menace && mainThreatZone >= 0) {
        int best = -1, bestToEnemy = 1000000000, bestBack = -1;
        if (pathMultiZone >= 0) {
            best = pathMultiZone;
            bestToEnemy = hopDist[pathMultiZone][mainThreatZone];
            bestBack = hopDist[pathMultiZone][myHQ];
        }
        for (auto& kv : bld) {
            if (kv.second.owner != meC) continue;
            int z = kv.first;
            int back = hopDist[z][myHQ];
            if (pathMultiZone >= 0) continue;
            bool urgentHQ = bigEnemyField || mainThreatCnt >= naturalHold;
            if (urgentHQ) {
                if (back != mainThreatEta && back != mainThreatEta - 1) continue;
            }
            else {
                if (back != mainThreatEta && back != mainThreatEta + 1) continue;
            }
            int toEnemy = hopDist[z][mainThreatZone];
            if (toEnemy < bestToEnemy || (toEnemy == bestToEnemy && back > bestBack)) {
                best = z; bestToEnemy = toEnemy; bestBack = back;
            }
        }


        if (best >= 0 && best != pathMultiZone && prevRallyZone >= 0 && prevRallyZone != best &&
            bld.count(prevRallyZone) && bld[prevRallyZone].owner == meC &&
            hopDist[prevRallyZone][myHQ] < 1000000000 && hopDist[prevRallyZone][mainThreatZone] < 1000000000) {
            int pb = hopDist[prevRallyZone][myHQ];
            bool urgentHQ = bigEnemyField || mainThreatCnt >= naturalHold;
            bool backOk = urgentHQ ? (pb <= mainThreatEta) : (pb <= mainThreatEta + 1);
            if (backOk && hopDist[prevRallyZone][mainThreatZone] <= hopDist[best][mainThreatZone] + 1)
                best = prevRallyZone;
        }
        if (best >= 0) rallyZone = best;
    }
    prevRallyZone = rallyZone;

    int interceptZone = -1, interceptNeed = 0;

    if (menace && bigEnemyField && pathMultiZone < 0 && (predTarget < 0 || predTarget == myHQ) &&
        mainThreatZone >= 0 && mainThreatEta < 1000000000) {
        int bestScore = -1000000000;
        for (int z = 0; z < N; z++) {
            if (bld.count(z) && bld[z].owner == oppC) continue;
            if (hopDist[mainThreatZone][z] + hopDist[z][myHQ] != mainThreatEta) continue;
            int eArr = hopDist[mainThreatZone][z];
            if (eArr <= 0) continue;
            int bestOur = 1000000000;
            for (auto& kv : war) {
                if (kv.second.owner != meC) continue;
                int from = kv.second.moving ? kv.second.target : kv.second.zone;
                if (from >= 0 && from < N) bestOur = min(bestOur, hopDist[from][z]);
            }
            if (bestOur > eArr) continue;
            int towardEnemy = hopDist[mainThreatZone][z];
            int awayFromHQ = hopDist[z][myHQ];
            int score = towardEnemy * 100 + awayFromHQ;
            if (score > bestScore) { bestScore = score; interceptZone = z; }
        }
        if (interceptZone >= 0)
            interceptNeed = max(0, eActiveField - hqTur + 1);
    }


    bool counterRace = false;
    if (bigEnemyField && mainThreatZone >= 0 && mainThreatEta < 1000000000 && bld.count(oppHQ) &&
        hopDist[rallyZone][oppHQ] < 1000000000) {
        bool cannotHold = (eActiveField - hqTur + 1) > myArmy;
        int oppHl = bld[oppHQ].level;
        int oppDef = oppInZone(oppHQ) + adjEnemyField(oppHQ) + HQ_TUR[oppHl];
        int myStrike = myArmy - min(myArmy, totalWorkCap());
        if (cannotHold && myStrike >= max(5, oppDef + 1)) {
            int ourKill = hopDist[rallyZone][oppHQ]
                + (oppDef + HQ_HP[oppHl] + myStrike - 1) / max(1, myStrike);
                int ballKill = mainThreatEta + naturalHold
                    + (HQ_HP[hl] + eActiveField - 1) / max(1, eActiveField);
                if (ourKill <= ballKill) counterRace = true;
        }
    }
    if (counterRace) { interceptZone = -1; interceptNeed = 0; }

    int defZone = rallyZone, needStaff = 0, worstOver = 0, hqOver = 0, hqNeed = 0;
    map<int, int> multiHold;
    set<int> evacuate;
    for (auto& kv : bld) {
        if (kv.second.owner != meC) continue;
        int z = kv.first; bool isHQ = kv.second.hq;

        set<int> near2;
        if (!isHQ) { near2.insert(z); for (int a1 : adj[z]) { near2.insert(a1); for (int a2 : adj[a1]) near2.insert(a2); } }
        int near = 0;
        if (isHQ) {


            near = (menace && (predTarget < 0 || predTarget == myHQ || mainThreatEta <= 2)) ? eActiveField : 0;
        }
        else {
            for (auto& w : war) {
                if (w.second.owner != oppC) continue;
                int wz = w.second.zone; if (wz < 0 || wz >= N) continue;
                if (bld.count(wz) && bld[wz].owner == oppC) continue;
                if (near2.count(wz) > 0) near++;
            }
            if (z == predTarget && predCnt > near) near = predCnt;
        }
        int tur = isHQ ? HQ_TUR[kv.second.level] : BS_TUR[kv.second.level];
        int over = near - (tur + bldLabCap(kv.second));
        if (over <= 0) continue;
        int need = (near - tur) + 1;
        if (isHQ) { hqOver = over; hqNeed = need; }
        else {


            int eEta = 1000000000;
            for (int q = 0; q < N; q++) if (enemyFieldCnt[q] > 0 && hopDist[q][z] < eEta) eEta = hopDist[q][z];
            int reachable = 0;
            for (auto& w : war) {
                if (w.second.owner != meC) continue;
                int from = w.second.moving ? w.second.target : w.second.zone;
                if (from >= 0 && from < N && hopDist[from][z] < 1000000000 &&
                    (eEta >= 1000000000 || hopDist[from][z] <= eEta + 2) &&
                    !pathUnsafe(from, z)) reachable++;
            }
            bool canGather = (reachable >= need);


            if (canGather && need <= ((near >= eActiveField) ? myArmy : max(4, myArmy / 2))) {

                if (buildDay.count(z) && day - buildDay[z] <= 3) multiHold[z] = need;
                if (over > worstOver) { worstOver = over; defZone = z; needStaff = need; }
            }


            else evacuate.insert(z);
        }
    }
    if (mainThreatZone >= 0 && needStaff == 0 && hopDist[rallyZone][mainThreatZone] <= 1 && !bld.count(mainThreatZone)) {
        int defC = mainThreatCnt + adjEnemyField(mainThreatZone);
        if (myArmy >= defC + 1) { defZone = mainThreatZone; needStaff = defC + 1; worstOver = max(worstOver, 1); }
    }
    if (interceptNeed > 0) {
        defZone = interceptZone;
        needStaff = max(needStaff, interceptNeed);
        worstOver = max(worstOver, eActiveField - naturalHold);
    }

    if (hqOver > 0) {
        defZone = (interceptNeed > 0) ? interceptZone : ((mainThreatEta <= 1) ? myHQ : rallyZone);
        needStaff = hqNeed;
        if (interceptNeed > 0) needStaff = max(needStaff, interceptNeed);
        worstOver = max(worstOver, hqOver);
    }

    if (counterRace) { defZone = rallyZone; needStaff = 0; worstOver = 0; hqOver = 0; hqNeed = 0; multiHold.clear(); }


    if (day >= 185 && bld.count(oppHQ) && myArmy >= 30) {
        int turnsLeft = 200 - day;
        vector<pair<int, string>> launch;
        int reachNow = 0, siegeTurns = 0;
        for (auto& kv : war) {
            if (kv.second.owner != meC || kv.second.moving) continue;
            int z = kv.second.zone;
            if (z < 0 || z >= N || hopDist[z][oppHQ] >= 1000000000) continue;
            int eta = hopDist[z][oppHQ];
            if (eta > turnsLeft) continue;
            launch.push_back({ eta, kv.first });
            reachNow++;
            siegeTurns += max(0, turnsLeft - eta + 1);
        }
        int hqHpLeft = bld[oppHQ].hp;
        bool enoughMass = reachNow >= max(30, eArmyTotal + 4);
        bool enoughDamage = siegeTurns >= hqHpLeft + 8;
        bool finalClock = (turnsLeft <= 12 && reachNow >= 22 && myArmy >= eArmyTotal);
        if (enoughMass || enoughDamage || finalClock) {
            sort(launch.begin(), launch.end());
            for (auto& pr : launch) {
                const string& w = pr.second;
                if (war[w].zone == oppHQ) continue;
                if (!sendTo(w, oppHQ)) break;
            }
            lastEnemyZone.clear();
            for (auto& kv : war)
                if (kv.second.owner == oppC) lastEnemyZone[kv.first] = kv.second.zone;
            cout << "END\n" << flush;
            return;
        }
    }

    bool forceEarlyTwoExpansion = (day <= 2 && nBases == 0 && !earlyRush);
    int earlyFirstBuildEta = 1000000000;
    if (forceEarlyTwoExpansion) {
        for (int z : strongOrder) {
            if (bld.count(z) || staffOf(z) > 0) continue;
            if (razeCnt[z] >= 2 && !dominate) continue;
            if (!canBuildSite(z)) continue;
            if (dist[myHQ][z] > dist[oppHQ][z] * expandReach) continue;
            if (oppInZone(z) > 0) continue;
            earlyFirstBuildEta = min(earlyFirstBuildEta, hopHQ[z] >= 1000000000 ? 0 : hopHQ[z]);
        }
    }
    auto projectedGoldOnArrival = [&](int z, int plannedBefore = 0) {
        int eta = hopHQ[z] >= 1000000000 ? 0 : hopHQ[z];
        long long projected = gold + currentIncome() * eta - 2LL * myCount() * eta - 300LL * plannedBefore;
        if (forceEarlyTwoExpansion && plannedBefore >= 1 && earlyFirstBuildEta < 1000000000)
            projected += 15LL * max(0, eta - earlyFirstBuildEta);
        return projected;
        };


    auto earlyTimingScore = [&](int z, int plannedBefore) {
        long long perTurn = max(1LL, currentIncome() + 15LL - 2LL * myCount());
        long long projected = projectedGoldOnArrival(z, plannedBefore);
        long long waitTurns = projected >= 300 ? 0 : (300 - projected + perTurn - 1) / perTurn;
        int hop = (hopHQ[z] >= 1000000000 ? 0 : hopHQ[z]);
        return tuple<int, long long, double, int>(hop, waitTurns, dist[myHQ][z], z);
        };
    auto canFundBaseOnArrival = [&](int z, int plannedBefore = 0) {
        long long perTurn = max(1LL, currentIncome() + 15LL - 2LL * myCount());
        return projectedGoldOnArrival(z, plannedBefore) + perTurn * 2 >= 300;
        };


    auto clusterKeyOf = [&](int z) {
        int coreBest = (hopHQ[z] >= 1000000000) ? 1000000000 : hopHQ[z];
        int anyBest = coreBest;
        for (auto& kv : bld) {
            if (kv.second.owner != meC) continue;
            int d = hopDist[kv.first][z];
            if (d >= 1000000000) continue;
            bool core = kv.second.hq || dist[myHQ][kv.first] <= dist[oppHQ][kv.first];
            if (core) coreBest = min(coreBest, d);
            anyBest = min(anyBest, d);
        }
        return pair<int, int>(coreBest, anyBest);
        };
    vector<int> expansionOrder = strongOrder;
    stable_sort(expansionOrder.begin(), expansionOrder.end(), [&](int a, int b) {
        auto ca = clusterKeyOf(a), cb = clusterKeyOf(b);
        if (ca.first != cb.first) return ca.first < cb.first;
        if (K >= 10 && fabs(dist[centerZone][a] - dist[centerZone][b]) > 1e-7)
            return dist[centerZone][a] < dist[centerZone][b];
        if (ca.second != cb.second) return ca.second < cb.second;
        if (hopHQ[a] != hopHQ[b]) return hopHQ[a] < hopHQ[b];
        return dist[myHQ][a] < dist[myHQ][b];
        });
    if (forceEarlyTwoExpansion) {
        stable_sort(expansionOrder.begin(), expansionOrder.end(), [&](int a, int b) {
            return earlyTimingScore(a, 1) < earlyTimingScore(b, 1);
            });
        if (!strongOrder.empty()) {
            int first = -1;
            for (int z : strongOrder) {
                if (bld.count(z) || staffOf(z) > 0) continue;
                if (razeCnt[z] >= 2 && !dominate) continue;
                if (!canBuildSite(z)) continue;
                if (dist[myHQ][z] > dist[oppHQ][z] * expandReach) continue;
                if (oppInZone(z) > 0) continue;
                first = z; break;
            }
            if (first >= 0) {
                expansionOrder.erase(remove(expansionOrder.begin(), expansionOrder.end(), first), expansionOrder.end());
                expansionOrder.insert(expansionOrder.begin(), first);
            }
        }
    }
    int buildBudget = 0;
    for (int z : expansionOrder) {
        if (bld.count(z)) continue;
        if (razeCnt[z] >= 2 && !dominate) continue;
        if (!canBuildSite(z)) continue;
        if (dist[myHQ][z] > dist[oppHQ][z] * expandReach) continue;
        if (oppInZone(z) > 0 || adjEnemyField(z) > 0) continue;
        if (!canFundBaseOnArrival(z, buildBudget)) break;
        buildBudget++;
    }
    buildBudget = max(0, min(buildBudget, (int)(max(0LL, avail) / 300) + 2));
    int buildPursuit = 0;


    map<int, int> kept;
    vector<string> freeW;
    for (auto& kv : war) {
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        int committed = committedTargetLive(kv.first);
        if (committed >= 0) {
            if (z == committed) { kept[z]++; continue; }
            sendTo(kv.first, committed);
            continue;
        }
        if (z == myHQ) {
            int lim = max(1, HQ_LAB[hl]);
            if (hqOver > 0) lim = max(lim, hqNeed);
            if (kept[z] < lim) { kept[z]++; continue; }
        }
        else if (bld.count(z) && bld[z].owner == meC) {
            int lim = bldLabCap(bld[z]);
            if (evacuate.count(z)) lim = 0;
            else if (z == defZone && needStaff > 0) lim = max(lim, needStaff);
            else if (multiHold.count(z)) lim = max(lim, multiHold[z]);
            if (kept[z] < lim) { kept[z]++; continue; }
        }
        else if (isBase[z]) {
            bool enemyBldHere = bld.count(z) && bld[z].owner == oppC;
            bool winning = myInZone(z) > oppInZone(z);
            if (shouldWatchRazedEnemyBase(z) && kept[z] < 1) { kept[z]++; continue; }


            int eTur = enemyBldHere ? (bld[z].hq ? HQ_TUR[bld[z].level] : BS_TUR[bld[z].level]) : 0;

            if (enemyBldHere && !bld[z].hq && oppInZone(z) == 0 && (day >= 150 || myInZone(z) >= 2)) {
                kept[z]++; continue;
            }
            if ((enemyBldHere && myInZone(z) > oppInZone(z) + eTur) ||
                (!enemyBldHere && oppInZone(z) > 0 && winning)) { kept[z]++; continue; }


            if (oppInZone(z) == 0 && adjEnemyField(z) == 0 &&
                !bld.count(z) && !(earlyRush && nBases >= 1) &&
                !(armyBehind && nBases >= 2) &&
                !(razeCnt[z] >= 2 && !dominate) &&
                canBuildSite(z) &&
                dist[myHQ][z] <= dist[oppHQ][z] * expandReach) {
                if (kept[z] < 1 && buildPursuit < buildBudget) { kept[z]++; buildPursuit++; continue; }
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
        if (counterRace) break;
        if (kv.second.owner != meC || kv.second.hq) continue;
        int z = kv.first;


        if (dist[myHQ][z] > dist[oppHQ][z]) continue;
        if (evacuate.count(z)) continue;
        if (adjEnemyField(z) > 0) continue;
        if (pathUnsafe(myHQ, z)) continue;
        if (oppInZone(z) > 0 || staffOf(z) >= bldLabCap(kv.second)) continue;
        if (war[freeW[fi]].zone != myHQ) break;
        sendTo(freeW[fi++], z);
    }


    if (needStaff > 0) {
        int have = staffOf(defZone);
        bool lastStand = (defZone == myHQ);


        bool anchored = bld.count(defZone) && bld[defZone].owner == meC;
        bool streamDef = lastStand || (anchored && needStaff <= myArmy);
        int enemyEtaToDef = 1000000000;
        for (int z2 = 0; z2 < N; z2++)
            if (enemyFieldCnt[z2] > 0 && hopDist[z2][defZone] < enemyEtaToDef) enemyEtaToDef = hopDist[z2][defZone];
        int wave = 0;
        for (size_t k = fi; k < freeW.size(); k++) {
            int z = war[freeW[k]].zone;
            if (z == defZone) continue;
            if (lastStand || z == rallyZone) wave++;
        }
        if (streamDef || have + wave >= needStaff) {
            while (fi < freeW.size() && have < needStaff) {
                size_t best = freeW.size(); double bd = 1e18;
                for (size_t k = fi; k < freeW.size(); k++) {
                    int z = war[freeW[k]].zone;
                    if (!streamDef && z != rallyZone && z != defZone) continue;
                    if (streamDef && !lastStand && z != defZone &&
                        hopDist[z][defZone] < 1000000000 &&
                        hopDist[z][defZone] > enemyEtaToDef + 3) continue;
                    if (!lastStand && z != defZone && pathUnsafe(z, defZone)) continue;
                    double d = dist[z][defZone];
                    if (d < bd) { bd = d; best = k; }
                }
                if (best == freeW.size()) break;
                swap(freeW[fi], freeW[best]);
                if (war[freeW[fi]].zone == defZone) { fi++; have = staffOf(defZone); continue; }
                if (!sendTo(freeW[fi], defZone)) break;
                fi++; have++;
            }
            if (have < needStaff && lastStand) {
                vector<pair<double, string>> rec;
                for (auto& kv : war) {
                    if (kv.second.owner != meC || kv.second.moving) continue;
                    int z = kv.second.zone;
                    if (z == defZone || z < 0 || z >= N) continue;
                    if (bld.count(z) && bld[z].owner == meC && !bld[z].hq &&
                        myInZone(z) <= bldLabCap(bld[z])) continue;
                    rec.push_back({ dist[z][defZone], kv.first });
                }
                sort(rec.begin(), rec.end());
                for (auto& pr : rec) { if (have >= needStaff) break; if (!sendTo(pr.second, defZone)) break; have++; }
            }
        }
    }


    if (centerBase >= 0 && !bld.count(centerBase) && staffOf(centerBase) == 0 &&
        !earlyRush && !armyBehind && !counterRace && !defenseModeCore &&
        razeCnt[centerBase] < 2 && oppInZone(centerBase) == 0 && adjEnemyField(centerBase) == 0 &&
        canBuildSite(centerBase) && fi < freeW.size()) {
        size_t best = freeW.size(); int bd = 1000000000;
        for (size_t k = fi; k < freeW.size(); k++) {
            int from = war[freeW[k]].zone;
            if (from < 0 || from >= N || hopDist[from][centerBase] >= 1000000000) continue;
            if (pathUnsafe(from, centerBase)) continue;
            if (hopDist[from][centerBase] < bd) { bd = hopDist[from][centerBase]; best = k; }
        }
        if (best != freeW.size()) {
            swap(freeW[fi], freeW[best]);
            if (sendTo(freeW[fi], centerBase)) { fi++; buildPursuit++; }
        }
    }


    for (int z : expansionOrder) {
        if (fi >= freeW.size() || avail < 10) break;
        if (buildPursuit >= buildBudget) break;
        if (earlyRush && nBases >= 1) break;
        if (armyBehind && nBases >= 2) break;
        if (counterRace) break;
        if (bld.count(z)) continue;
        if (razeCnt[z] >= 2 && !dominate) continue;
        if (!canBuildSite(z)) continue;
        if (dist[myHQ][z] > dist[oppHQ][z] * expandReach) continue;
        if (oppInZone(z) > 0 || adjEnemyField(z) > 0) continue;
        if (staffOf(z) >= 1) continue;
        if (pathUnsafe(war[freeW[fi]].zone, z)) continue;
        if (!canFundBaseOnArrival(z, buildPursuit)) break;
        if (!sendTo(freeW[fi], z)) break;
        fi++; buildPursuit++;
    }


    if (day >= 145 && !menace && !armyBehind && fi < freeW.size() && avail >= 10 && gold >= 260 &&
        (majorityBases || myArmy >= eArmyTotal + 8 || oppMultiValue >= myMultiValue)) {
        bool forwardForcePath = (myArmy >= eArmyTotal + 12);
        int bestZ = -1;
        long long bestScore = LLONG_MIN;
        size_t bestK = freeW.size();
        for (int z : strongOrder) {
            if (!isBase[z] || bld.count(z) || staffOf(z) > 0 || oppInZone(z) > 0 || adjEnemyField(z) > 0) continue;
            if (razeCnt[z] >= 2 && !dominate) continue;
            if (dist[oppHQ][z] < dist[myHQ][oppHQ] * 0.25) continue;
            if (dist[myHQ][z] > dist[oppHQ][z] * 1.45) continue;
            for (size_t k = fi; k < freeW.size(); k++) {
                int from = war[freeW[k]].zone;
                if (from < 0 || from >= N) continue;
                int eta = hopDist[from][z];
                if (eta >= 1000000000) continue;
                if (!forwardForcePath && pathUnsafe(from, z)) continue;
                int payback = eta + 20 + 4;
                if (200 - day <= payback) continue;
                long long centerBonus = -90LL * dist[centerZone][z];
                long long pressure = 20LL * dist[myHQ][z] - 12LL * dist[oppHQ][z];
                long long score = centerBonus + pressure - 80LL * eta - 250LL * razeCnt[z];
                if (score > bestScore) {
                    bestScore = score;
                    bestZ = z;
                    bestK = k;
                }
            }
        }
        if (bestZ >= 0 && bestK < freeW.size()) {
            swap(freeW[fi], freeW[bestK]);
            sendTo(freeW[fi++], bestZ);
            buildPursuit++;
        }
    }

    for (auto& pr : enemyRazeWatchUntil) {
        int z = pr.first;
        if (fi >= freeW.size() || avail < 10) break;
        if (!shouldWatchRazedEnemyBase(z) || staffOf(z) > 0) continue;
        size_t best = freeW.size(); int bd = 1000000000;
        for (size_t k = fi; k < freeW.size(); k++) {
            int from = war[freeW[k]].zone;
            if (hopDist[from][z] > 3 || pathUnsafe(from, z)) continue;
            if (hopDist[from][z] < bd) { bd = hopDist[from][z]; best = k; }
        }
        if (best != freeW.size()) {
            swap(freeW[fi], freeW[best]);
            if (sendTo(freeW[fi], z)) fi++;
        }
    }


    if (fi < freeW.size()) {


        int eMassZone = -1, eMassCnt = 0;
        {
            map<int, int> ec;
            for (auto& kv : war) {
                if (kv.second.owner != oppC) continue;
                int z = kv.second.zone; if (z < 0 || z >= N) continue;
                if (++ec[z] > eMassCnt) { eMassCnt = ec[z]; eMassZone = z; }
            }
        }
        int freeAvailAtStart = (int)(freeW.size() - fi);
        bool lateCrackEconomy = (day >= 150 && (strategicBehind || oppStrategicValue == myStrategicValue || oppBases >= nBases));
        auto enemyMultiLossValue = [&](const Bld& eb) { return eb.level * 2 + bldLabCap(eb); };
        auto crackToParityByRazing = [&](const Bld& eb) {
            if (eb.hq || oppStrategicValue <= myStrategicValue) return false;
            return oppStrategicValue - enemyMultiLossValue(eb) <= myStrategicValue;
            };
        int rallyStack = staffOf(rallyZone);
        int attackPoolHint = max(rallyStack, freeAvailAtStart);
        bool deathballReady = (day >= 120 || expandDone || majorityBases) &&
            (myArmy >= max(25, eArmyTotal + 12) || attackPoolHint >= max(25, threatArmy + 10));
        bool forceAttackPath = deathballReady;

        int tgt = -1;
        {
            long long bestKey = LLONG_MIN, prevKey = LLONG_MIN;
            int fallback = -1; double fbD = 1e18;
            int freeAvail0 = freeAvailAtStart;
            for (auto& kv : bld) {
                if (kv.second.owner != oppC || dist[myHQ][kv.first] >= 1e17) continue;
                int z = kv.first;
                if (!forceAttackPath && pathUnsafe(rallyZone, z)) continue;
                if (dist[myHQ][z] < fbD) { fbD = dist[myHQ][z]; fallback = z; }
                int tl = kv.second.level;
                int adjE = adjEnemyField(z);
                bool blockedAtk = !kv.second.hq && !safeExpansionLeft && econBehind;
                bool parityCrack = crackToParityByRazing(kv.second);
                int mag = kv.second.hq ? 12 : ((oppBases > nBases || blockedAtk || parityCrack) ? 2 : 5);
                int req = kv.second.hq
                    ? max(mag, oppInZone(z) + adjE + HQ_TUR[tl] + HQ_LAB[tl] + 4)
                    : max(mag, oppInZone(z) + adjE + tl + 1);
                if (!dominate && staffOf(z) + freeAvail0 < req) continue;
                int ourEta = (hopDist[rallyZone][z] >= 1000000000) ? 999 : hopDist[rallyZone][z];
                int massEta = (eMassZone < 0 || hopDist[eMassZone][z] >= 1000000000) ? 999 : hopDist[eMassZone][z];
                long long key = (long long)(massEta - ourEta) * 1000 - ourEta;
                if (lateCrackEconomy && !kv.second.hq) key += 6000 + 1500LL * kv.second.level;
                if (parityCrack) key += 14000 + 2500LL * enemyMultiLossValue(kv.second);
                if (centerBase >= 0 && z == centerBase && !kv.second.hq) key += 8000;
                if (z == prevAttackTgt) prevKey = key;
                if (key > bestKey) { bestKey = key; tgt = z; }
            }

            if (prevKey > LLONG_MIN && prevKey >= bestKey - 2000) tgt = prevAttackTgt;
            if (tgt < 0) tgt = fallback;
        }
        prevAttackTgt = tgt;

        int clTgt = -1, clE = 0;
        for (int z = 0; z < N; z++) {
            if (bld.count(z)) continue;
            int e = oppInZone(z);
            if (e > 0 && dist[myHQ][z] <= dist[oppHQ][z] && e > clE && (forceAttackPath || !pathUnsafe(rallyZone, z))) { clE = e; clTgt = z; }
        }


        auto reinforceTo = [&](int dest, int req, bool checkPath = true) {
            int have = staffOf(dest);
            while (fi < freeW.size() && avail >= 10 && have < req) {
                size_t pick = freeW.size();
                for (size_t k = fi; k < freeW.size(); k++) {
                    int from = war[freeW[k]].zone;
                    if (from == dest) { pick = k; break; }
                    if (checkPath && pathUnsafe(from, dest)) continue;
                    pick = k; break;
                }
                if (pick == freeW.size()) break;
                swap(freeW[fi], freeW[pick]);
                if (war[freeW[fi]].zone == dest) { fi++; have = staffOf(dest); continue; }
                if (!sendTo(freeW[fi], dest)) break;
                fi++; have++;
            }
            };


        bool launched = false;


        if (counterRace) {
            reinforceTo(oppHQ, (int)freeW.size(), false);
            launched = true;
        }


        bool attackAllowed = (!menace && !bigEnemyField) || deathballReady;
        if (!launched && attackAllowed) {

            if (!launched && clTgt >= 0) {
                int defC = clE + adjEnemyField(clTgt);
                int req = max(5, defC + 1);
                int atT = staffOf(clTgt), freeAvail = (int)(freeW.size() - fi);
                bool mustered = (atT + freeAvail) >= req;
                bool pressing = atT > defC;
                if (dominate || mustered || pressing) {
                    reinforceTo(clTgt, dominate ? (atT + freeAvail) : req, !forceAttackPath);
                    launched = true;
                }
            }

            if (!launched && tgt >= 0) {
                int tl = bld[tgt].level;
                int adjE = adjEnemyField(tgt);
                bool expansionBlockedAttack = !bld[tgt].hq && !safeExpansionLeft && econBehind;
                bool parityCrack = !bld[tgt].hq && crackToParityByRazing(bld[tgt]);
                int minAttackGroup = bld[tgt].hq ? 12 : ((!bld[tgt].hq && (oppBases > nBases || expansionBlockedAttack || parityCrack)) ? 2 : 5);
                int req = bld[tgt].hq
                    ? max(minAttackGroup, oppInZone(tgt) + adjE + HQ_TUR[tl] + HQ_LAB[tl] + 4)
                    : max(minAttackGroup, oppInZone(tgt) + adjE + tl + 1);
                int atT = staffOf(tgt), freeAvail = (int)(freeW.size() - fi);
                int defT = oppInZone(tgt) + adjE + (bld[tgt].hq ? HQ_TUR[tl] : BS_TUR[tl]);
                bool mustered = (atT + freeAvail) >= req;
                bool pressing = !bld[tgt].hq && atT > defT;
                if (dominate || mustered || pressing) {
                    reinforceTo(tgt, dominate ? (atT + freeAvail) : req, !forceAttackPath);
                    launched = true;
                }
            }
        }


        int funnelTo = (defZone == myHQ) ? myHQ : rallyZone;
        int funnelCap = max(needStaff + 4, max(12, mainThreatCnt + 4));
        if (deathballReady || day >= 180) funnelCap = max(funnelCap, mainThreatCnt + 8);
        while (fi < freeW.size()) {
            if (staffOf(funnelTo) >= funnelCap) break;
            if (war[freeW[fi]].zone == funnelTo) { fi++; continue; }
            if (funnelTo != myHQ && !(bld.count(funnelTo) && bld[funnelTo].owner == meC) && avail < 10) break;
            if (pathUnsafe(war[freeW[fi]].zone, funnelTo)) { fi++; continue; }
            if (!sendTo(freeW[fi], funnelTo)) break;
            fi++;
        }

        if (fi < freeW.size() && tgt >= 0 && (lateCrackEconomy || day >= 170 || deathballReady) && hqOver == 0) {
            int tl = bld[tgt].level;
            int adjE = adjEnemyField(tgt);
            int minAttackGroup = bld[tgt].hq ? 12 : 5;
            int req = bld[tgt].hq
                ? max(minAttackGroup, oppInZone(tgt) + adjE + HQ_TUR[tl] + HQ_LAB[tl] + 4)
                : max(minAttackGroup, oppInZone(tgt) + adjE + tl + 1);
            int atT = staffOf(tgt), freeAvail = (int)(freeW.size() - fi);
            if (dominate || deathballReady || atT + freeAvail >= req) {
                reinforceTo(tgt, dominate || deathballReady ? (atT + freeAvail) : req, !(deathballReady || lateCrackEconomy));
            }
        }
    }


    lastEnemyZone.clear();
    for (auto& kv : war)
        if (kv.second.owner == oppC) lastEnemyZone[kv.first] = kv.second.zone;

    cout << "END\n" << flush;
}


void parseResult() {
    int T; cin >> T;
    string sec; cin >> sec;
    long long Tx, Rx, Ty, Ry; cin >> Tx >> Rx >> Ty >> Ry;
    byoyomi = (int)Rx;

    vector<pair<char, int>> ups;
    vector<string> trains;
    vector<pair<string, int>> moves;
    vector<tuple<string, string, int>> dmg;
    vector<tuple<char, int, int>> sieges;

    while (cin >> sec) {
        if (sec == "END") break;
        int n; cin >> n;
        if (sec == "UPGRADE") {
            for (int i = 0; i < n; i++) { string t; int d; cin >> t >> d; ups.push_back({ t[0], d }); }
        }
        else if (sec == "TRAIN") {
            for (int i = 0; i < n; i++) { string id; cin >> id; trains.push_back(id); }
        }
        else if (sec == "MOVE") {
            for (int i = 0; i < n; i++) { string id; int d; cin >> id >> d; moves.push_back({ id, d }); }
        }
        else if (sec == "DAMAGE") {
            for (int i = 0; i < n; i++) { string c, id; int h; cin >> c >> id >> h; dmg.push_back({ c, id, h }); }
        }
        else if (sec == "SIEGE") {
            for (int i = 0; i < n; i++) { string t; int d, h; cin >> t >> d >> h; sieges.push_back({ t[0], d, h }); }
        }
        else {
            for (int i = 0; i < n; i++) { string junk; getline(cin, junk); }
        }
    }


    for (auto& [t, d] : ups) {
        bool hqHere = bld.count(d) && bld[d].hq;
        if (t == meC) {
            enemyRazeWatchUntil.erase(d);
            if (!bld.count(d) || (bld[d].owner != meC && !hqHere)) bld[d] = { meC, 1, BS_HP[1], false };
            continue;
        }
        if (!bld.count(d) || (bld[d].owner != t && !hqHere)) {
            if (bld.count(d) && bld[d].owner == meC && !bld[d].hq) razeCnt[d]++;
            enemyRazeWatchUntil.erase(d);
            bld[d] = { t, 1, BS_HP[1], false };
        }
        else {
            Bld& b = bld[d];
            int mx = b.hq ? 5 : 3;
            if (b.level < mx) b.level++;
            b.hp = bldMaxHP(b);
        }
    }

    for (auto& id : trains) {
        char ow = id[0];
        war[id] = { ow, hqZoneOf(ow), HQ_WHP[hqLevelOf(ow)], false, -1 };
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
    for (auto it = bld.begin(); it != bld.end();) {
        if (it->second.hp <= 0) {
            if (it->second.owner == meC && !it->second.hq) razeCnt[it->first]++;
            else if (it->second.owner == oppC && !it->second.hq) enemyRazeWatchUntil[it->first] = T + 12;
            it = bld.erase(it);
        }
        else ++it;
    }

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
