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

static const long long GOLD_BUFFER = 120;
static const double EXPAND_REACH = 1.30;
static const int ATTACK_SIZE = 6;
static const double DINF = 1e17;

int N, K;
vector<long long> PX, PY;
vector<char> isBase;
vector<vector<int>> adj;
vector<vector<double>> dist;
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
bool lastMyHqSieged = false;
int lastMyBaseSiegedZone = -1;
int lastMyBaseSiegedTurn = -1000000;

int hqZoneOf(char c) { return c == 'A' ? 0 : N - 1; }
bool validZone(int z) { return 0 <= z && z < N; }
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

bool reachable(int a, int b) {
    return validZone(a) && validZone(b) &&
           (int)dist.size() == N && (int)dist[a].size() == N && dist[a][b] < DINF;
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
    gold = 500;
    byoyomi = 5;
    strongOrder.clear();
    bld.clear();
    war.clear();
    razeCnt.clear();
    buildDay.clear();
    lastMyHqSieged = false;
    lastMyBaseSiegedZone = -1;
    lastMyBaseSiegedTurn = -1000000;
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

    centerBase = -1; { double cbD = 1e18;
        for (int z : strongOrder) {
            if (z == myHQ || dist[myHQ][z] > dist[oppHQ][z]) continue;
            if (dist[z][centerZone] < cbD) { cbD = dist[z][centerZone]; centerBase = z; }
        } }
    if (centerBase >= 0) {
        auto it = find(strongOrder.begin(), strongOrder.end(), centerBase);
        size_t idx = (size_t)(it - strongOrder.begin());

        if (idx > 1) { strongOrder.erase(it); strongOrder.insert(strongOrder.begin() + 1, centerBase); }
    }
    baseCap = 0;
    for (int z : strongOrder) if (dist[myHQ][z] <= dist[oppHQ][z] * EXPAND_REACH) baseCap++;

    cout << "OK\n" << flush;
}

void doCommands(int day) {
    cout << "COMMAND\n";
    if (!bld.count(myHQ)) { cout << "END\n" << flush; return; }
    long long avail = gold - GOLD_BUFFER;
    int hl = bld[myHQ].level;
    auto oppFieldInZone = [&](int z) {
        int c = oppInZone(z);
        auto it = bld.find(z);
        if (it != bld.end() && it->second.owner == oppC)
            c = max(0, c - bldLabCap(it->second));
        return c;
    };
    int eField = 0, eArmyTotal = 0, spearHop = INT_MAX, spearZone = -1;
    int hqRingThreat = 0;
    int hqOuterThreat = 0;
    vector<int> enemyFieldCnt(N, 0);
    for (auto& kv : war) {
        if (kv.second.owner != oppC) continue;
        int z = kv.second.zone;
        if (z < 0 || z >= N) continue;
        eArmyTotal++;
        bool worker = bld.count(z) && bld[z].owner == oppC;
        if (worker) continue;
        eField++;
        enemyFieldCnt[z]++;
        if (hopHQ[z] <= 3) hqRingThreat++;
        if (hopHQ[z] <= 4) hqOuterThreat++;
        if (hopHQ[z] < spearHop) { spearHop = hopHQ[z]; spearZone = z; }
    }
    int enemyClusterZone = -1, enemyClusterSize = 0;
    for (int z = 0; z < N; z++) {
        if (enemyFieldCnt[z] > enemyClusterSize ||
            (enemyFieldCnt[z] == enemyClusterSize && enemyClusterZone >= 0 && hopHQ[z] < hopHQ[enemyClusterZone])) {
            enemyClusterZone = z;
            enemyClusterSize = enemyFieldCnt[z];
        }
    }
    int myArmy = myCount();
    bool dominate = (myArmy >= eArmyTotal + 4);
    int hqTur = HQ_TUR[hl];
    int hqHopWidth = (oppHQ >= 0 && oppHQ < (int)hopHQ.size()) ? hopHQ[oppHQ] : 1000000000;

    int naturalHold = hqTur + HQ_LAB[hl];
    bool menace = false;
    if (eField > naturalHold && spearZone >= 0) {
        if (dist[myHQ][spearZone] <= dist[oppHQ][spearZone]) menace = true;
        else if (hqHopWidth < 1000000000 && spearHop * 10 <= hqHopWidth * 6) menace = true;
    }

    set<int> usedUpg;
    auto issueUpgrade = [&](int z, long long cost) -> bool {
        if (!validZone(z) || usedUpg.count(z)) return false;
        if (cost < 0 || gold < cost) return false;
        cout << "UPGRADE " << z << "\n";
        gold -= cost;
        avail -= cost;
        usedUpg.insert(z);
        return true;
    };
    auto issueTrain = [&](int count) -> bool {
        if (count <= 0) return false;
        count = min(count, HQ_TRAIN[hl]);
        long long cost = 120LL * count;
        if (gold < cost) return false;
        cout << "TRAIN " << count << "\n";
        gold -= cost;
        avail -= cost;
        return true;
    };

    int nBases = 0;
    for (auto& kv : bld) if (kv.second.owner == meC && !kv.second.hq) nBases++;
    bool expandDone = (nBases >= baseCap);
    bool econOK = expandDone || nBases >= 4;

    int oppBases = 0;
    for (auto& kv : bld) if (kv.second.owner == oppC && !kv.second.hq) oppBases++;
    bool earlyRush = (nBases < 2 && day <= 40 && oppBases == 0 && eArmyTotal >= 4);

    bool raidPressure = N >= 75 && day <= 85 && eField >= 5 && !dominate &&
                        (oppBases <= 2 || eField >= max(5, myArmy / 2));
    bool haMode = day >= 18 && day <= 175 && oppBases <= 2 &&
                  eArmyTotal >= 4 && (oppBases >= 1 || eField >= 3) &&
                  (enemyClusterSize >= 3 || eField >= 4 || eArmyTotal >= myArmy + 1);
    bool earlyHqIntrusion = oppInZone(myHQ) > 0 && day <= 95 && !dominate;
    bool hqSiegeMemory = lastMyHqSieged && !dominate;
    bool defenseAlert = earlyRush || raidPressure || hqSiegeMemory;
    if (raidPressure) menace = true;
    if (hqSiegeMemory) menace = true;

    bool smallMapRaceRisk = N < 70;
    bool lateHqPreRingScreen = !smallMapRaceRisk && day >= 170 && day <= 185 &&
                               oppInZone(myHQ) == 0 && hqRingThreat == 0 &&
                               hqOuterThreat >= 6 && spearHop == 4 &&
                               !dominate && myArmy >= 10 && eField >= 10 &&
                               nBases >= 4 && oppBases >= 4;
    bool lateHqRingLock = !smallMapRaceRisk && day >= 170 && day <= 188 &&
                          oppInZone(myHQ) == 0 && hqRingThreat >= 5 &&
                          spearHop <= 3 && !dominate && myArmy >= 10 && eField >= 10 &&
                          nBases >= 4 && oppBases >= 4 &&
                          (nBases >= oppBases || myArmy + 8 >= eArmyTotal);
    bool lateHqScreen = lateHqPreRingScreen || lateHqRingLock;
    if (lateHqScreen) menace = true;
    bool haPathIntercept = smallMapRaceRisk && day >= 45 && day <= 125 &&
                           spearZone >= 0 && spearZone != myHQ &&
                           spearHop <= 3 && hqRingThreat >= 1 &&
                           !dominate && eArmyTotal >= 5;
    bool finishMode = !defenseAlert &&
                      ((smallMapRaceRisk && day >= 155 &&
                        (myArmy >= eArmyTotal + 4 || nBases >= oppBases + 2 || day >= 185)) ||
                       (!smallMapRaceRisk && day >= 135 &&
                        (myArmy >= eArmyTotal + 1 || nBases >= oppBases + 1 || day >= 170)));
    bool homeQuietForFinish = !hqSiegeMemory && (spearZone < 0 || spearHop > 4 || dominate);
    bool finishAllIn = finishMode && homeQuietForFinish &&
                       ((smallMapRaceRisk &&
                         (myArmy >= eArmyTotal + 7 || nBases >= oppBases + 3 || day >= 195)) ||
                        (!smallMapRaceRisk &&
                         (myArmy >= eArmyTotal + 3 || nBases >= oppBases + 2 || day >= 175)));

    bool compactMap = N <= 81;
    bool tempoBehind = (day >= 45 && myArmy + 3 < eArmyTotal) ||
                       (day >= 80 && myArmy + 1 < eArmyTotal && oppBases <= nBases + 1);
    bool smallPressureTrain = smallMapRaceRisk && day >= 45 && day <= 125 &&
                              !defenseAlert &&
                              (myArmy + 2 < eArmyTotal ||
                               (oppBases <= nBases && myArmy <= eArmyTotal + 3));
    bool productionMode = finishMode || tempoBehind || haMode ||
                          earlyHqIntrusion ||
                          smallPressureTrain ||
                          (smallMapRaceRisk && day >= 100 && !defenseAlert &&
                           (myArmy <= eArmyTotal + 10 || nBases >= max(2, oppBases)));
    bool emergencyTrain = tempoBehind || haMode || defenseAlert || menace || hqSiegeMemory || lastMyHqSieged ||
                          smallPressureTrain ||
                          oppInZone(myHQ) > 0 ||
                          (day > 25 && myArmy + 2 < eArmyTotal);
    bool closeMode = (day >= 175 && oppInZone(myHQ) == 0 &&
                     (myArmy + 3 >= eArmyTotal || nBases >= oppBases + 1 || day >= 190));
    bool lateScoreMode = day >= 185 && oppInZone(myHQ) == 0 && !defenseAlert;
    bool hqChipMode = day >= 178 && oppInZone(myHQ) == 0 &&
                      (myArmy + 2 >= eArmyTotal || nBases >= oppBases || day >= 192);
    bool lateAllClearChip = day >= 180 && eArmyTotal == 0 && oppBases == 0 &&
                            oppInZone(myHQ) == 0 && bld.count(oppHQ) &&
                            bld[oppHQ].owner == oppC &&
                            bld[myHQ].hp <= bld[oppHQ].hp &&
                            myArmy >= 1;

    auto staffOf = [&](int z) {
        int c = 0;
        for (auto& kv : war)
            if (kv.second.owner == meC &&
                (kv.second.zone == z || (kv.second.moving && kv.second.target == z))) c++;
        return c;
    };

    auto adjEnemyField = [&](int z) {
        int c = 0;
        for (int a : adj[z]) c += oppFieldInZone(a);
        return c;
    };

    int openOwnWork = 0;
    for (auto& kv : bld) {
        if (kv.second.owner != meC) continue;
        openOwnWork += max(0, bldLabCap(kv.second) - staffOf(kv.first));
    }
    bool lateIncomeRecovery = smallMapRaceRisk && day >= 145 && gold < 300 &&
                              eField == 0 && nBases >= 1 && openOwnWork > 0;

    for (auto& kv : war) {
        if (gold < 300) break;
        if (compactMap && day >= 45 && nBases >= 4 && !dominate) break;
        if (earlyRush && nBases >= 1) break;
        if (raidPressure && nBases >= 2) break;
        if (haMode && nBases >= (compactMap ? 1 : 2) && !dominate) break;
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        if (z == myHQ || !isBase[z] || bld.count(z) || usedUpg.count(z)) continue;
        if (oppInZone(z) > 0) continue;

        if (razeCnt[z] >= 2 && !dominate) continue;

        if (dist[oppHQ][z] < dist[myHQ][oppHQ] * 0.20) continue;
        if (!issueUpgrade(z, 300)) continue;
        bld[z] = {meC, 1, BS_HP[1], false}; nBases++; buildDay[z] = day;
    }

    int eHQlv = bld.count(oppHQ) ? bld[oppHQ].level : 1;
    bool techCatchupSoon = N >= 90 && day >= 80 && nBases >= 7 && hl < 3 &&
                           eHQlv > hl && !haMode && !defenseAlert && !menace &&
                           oppInZone(myHQ) == 0 && myArmy + 12 >= eArmyTotal;
    bool techCatchupNow = techCatchupSoon && day >= 88 && myInZone(myHQ) >= 1;
    bool hqTieMode = day >= 145 && oppInZone(myHQ) == 0 && !defenseAlert &&
                     (hl <= eHQlv || day >= 185);
    bool lateHqHpTiebreak = day >= 175 && oppInZone(myHQ) == 0 && !defenseAlert &&
                            (oppBases == 0 || eArmyTotal == 0 ||
                             nBases >= oppBases + 2 || day >= 195) &&
                            myArmy + 4 >= eArmyTotal;
    long long lateHqHpCost = (hl < 5) ? HQ_UP[hl + 1] : HQ_REPAIR;
    bool lateHq2DrawBreak = hl == 1 && day >= 185 && oppInZone(myHQ) == 0 &&
                            !defenseAlert && hqRingThreat == 0 &&
                            bld[myHQ].hp == HQ_HP[hl] &&
                            nBases >= 2;
    bool finalHq2SpendBuffer = lateHq2DrawBreak && day >= 198 &&
                               gold >= HQ_UP[2] &&
                               (day >= 200 || oppBases == 0 || nBases >= oppBases + 2 || myArmy >= eArmyTotal + 4);
    bool finalTurnHqUpgradeUnderRing = day >= 200 && hl < 5 &&
                                       oppInZone(myHQ) == 0 && myInZone(myHQ) >= 1 &&
                                       bld[myHQ].hp == HQ_HP[hl] &&
                                       bld.count(oppHQ) && bld[oppHQ].owner == oppC &&
                                       hl <= bld[oppHQ].level &&
                                       gold >= HQ_UP[hl + 1];
    if (oppInZone(myHQ) == 0 && myInZone(myHQ) >= 1 && !usedUpg.count(myHQ)) {
        if (finalTurnHqUpgradeUnderRing) {
            if (issueUpgrade(myHQ, HQ_UP[hl + 1])) {
                bld[myHQ].level = ++hl;
                bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (finalHq2SpendBuffer) {
            if (issueUpgrade(myHQ, HQ_UP[2])) {
                bld[myHQ].level = ++hl;
                bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (lateHq2DrawBreak && avail >= HQ_UP[2] + 20) {
            if (issueUpgrade(myHQ, HQ_UP[2])) {
                bld[myHQ].level = ++hl;
                bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (lateHqHpTiebreak && bld[myHQ].hp < HQ_HP[hl] && avail >= lateHqHpCost + 40) {
            if (issueUpgrade(myHQ, lateHqHpCost)) {
                if (hl < 5) bld[myHQ].level = ++hl;
                bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (hl < 5 && (!haMode && (!menace || hqTieMode))) {
            long long need = HQ_UP[hl + 1];

            long long buffer = (hl < eHQlv) ? 80 : (expandDone ? 200 : ((hl < 3) ? 240 : 1500));
            if (techCatchupNow) buffer = min<long long>(buffer, 60);
            bool doUpgrade = (econOK && !tempoBehind && avail >= need + buffer) ||
                             (techCatchupNow && avail >= need + 40) ||
                             (hqTieMode && avail >= need + 80 && (myArmy + 4 >= eArmyTotal || day >= 185));
            if (doUpgrade && issueUpgrade(myHQ, need)) {
                bld[myHQ].level = ++hl; bld[myHQ].hp = HQ_HP[hl];
            }
        } else if (bld[myHQ].hp <= HQ_HP[5] - 5 && avail >= HQ_REPAIR + 600) {
            if (issueUpgrade(myHQ, HQ_REPAIR)) bld[myHQ].hp = HQ_HP[5];
        }
    }

    for (auto& kv : bld) {
        if (menace || tempoBehind || haMode) break;
        Bld& b = kv.second; int z = kv.first;
        if (b.owner != meC || b.hq || b.level >= 2) continue;
        if (dist[myHQ][z] > dist[oppHQ][z]) continue;
        if (usedUpg.count(z) || oppInZone(z) > 0 || myInZone(z) < bldLabCap(b)) continue;
        if (lateHqHpTiebreak && hl < 5 && avail < lateHqHpCost + 300) continue;
        if (lateHq2DrawBreak && avail < HQ_UP[2] + 300) continue;
        long long c = BS_COST[b.level + 1];
        if (avail >= c + (expandDone ? 200 : 800) && issueUpgrade(z, c)) {
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

        if (menace || tempoBehind || haMode || smallPressureTrain || (compactMap && day >= 70)) baseReserve = 0;
        int cap = HQ_TRAIN[hl];
        int twc = totalWorkCap();
        int units = myCount();
        int sustainMul = tempoBehind ? 10 : (productionMode ? 8 : 6);
        int sustain = sustainMul * max(1, twc);

        long long hqReserve = (lateHqHpTiebreak && hl < 5) ? (lateHqHpCost + 40) :
                              (lateHq2DrawBreak ? (HQ_UP[2] + 20) :
                              (techCatchupSoon ? (HQ_UP[hl + 1] + 40) :
                               ((tempoBehind || haMode) ? 0 : (((hl < eHQlv || day >= 175) && hl < 5) ? (HQ_UP[hl + 1] + 80) : 0))));
        if ((smallPressureTrain && day < 135) || earlyHqIntrusion) hqReserve = 0;
        long long trainable = avail - baseReserve - hqReserve;
        int want = 0;

        long long pileLimit = tempoBehind ? 0 : (productionMode ? 360 : 1200);
        if (trainable > 0 && (tempoBehind || units < sustain || trainable > pileLimit)) want = min(cap, (int)(trainable / 120));

        if ((nBases < 1 && !emergencyTrain) ||
            (nBases < 2 && !defenseAlert && !haMode && !earlyHqIntrusion && day <= 25)) want = 0;

        while (want > 0 && 120LL * want > avail) want--;
        issueTrain(want);
    }

    auto sendTo = [&](const string& w, int dest) -> bool {
        auto it = war.find(w);
        if (it == war.end()) return false;
        War& unit = it->second;
        if (unit.owner != meC || unit.moving) return false;
        if (!validZone(unit.zone) || !validZone(dest) || unit.zone == dest) return false;
        if (!reachable(unit.zone, dest)) return false;
        long long mc = (bld.count(dest) && bld[dest].owner == meC) ? 0 : 10;
        if (gold < mc) return false;
        cout << "MOVE " << w << " " << dest << "\n";
        gold -= mc; avail -= mc; unit.moving = true; unit.target = dest;
        return true;
    };

    int rallyZone = myHQ; { double rb = 1e18;
        for (auto& kv : bld) {
            if (kv.second.owner != meC) continue;
            double sum = 0; int cnt = 0;
            for (auto& eb : bld) if (eb.second.owner == oppC && dist[kv.first][eb.first] < 1e17) { sum += dist[kv.first][eb.first]; cnt++; }
            double avg = cnt ? sum / cnt : dist[kv.first][oppHQ];
            if (avg < rb) { rb = avg; rallyZone = kv.first; } } }
    if (haMode) {
        int best = myHQ;
        double bestScore = 1e18;
        for (auto& kv : bld) {
            if (kv.second.owner != meC || kv.second.hq) continue;
            int z = kv.first;
            double front = bld.count(oppHQ) ? dist[z][oppHQ] : dist[z][N - 1];
            double back = dist[z][myHQ];
            double cluster = enemyClusterZone >= 0 ? dist[z][enemyClusterZone] : front;
            double score = front * 0.55 + cluster * 0.35 + back * 0.10;
            if (score < bestScore) { bestScore = score; best = z; }
        }
        rallyZone = best;
    }
    int defZone = rallyZone, needStaff = 0, worstOver = 0, hqOver = 0, hqNeed = 0;
    map<int,int> multiHold;
    for (auto& kv : bld) {
        if (kv.second.owner != meC) continue;
        int z = kv.first; bool isHQ = kv.second.hq;

        set<int> near2;
        if (!isHQ) { near2.insert(z); for (int a1 : adj[z]) { near2.insert(a1); for (int a2 : adj[a1]) near2.insert(a2); } }
        int near = 0;
        for (auto& w : war) {
            if (w.second.owner != oppC) continue;
            int wz = w.second.zone; if (wz < 0 || wz >= N) continue;
            if (bld.count(wz) && bld[wz].owner == oppC) continue;
            if (isHQ ? (hopHQ[wz] <= 3) : (near2.count(wz) > 0)) near++;
        }
        int tur = isHQ ? HQ_TUR[kv.second.level] : BS_TUR[kv.second.level];

        int holdCap = (isHQ && day >= 120) ? staffOf(z) : bldLabCap(kv.second);
        int over = near - (tur + holdCap);
        if (over <= 0) continue;
        int need = (near - tur) + 1;
        if (isHQ) { hqOver = over; hqNeed = need; }

        else if (need <= max(4, myArmy / 2)) {

            if (buildDay.count(z) && day - buildDay[z] <= 3) multiHold[z] = need;
            if (over > worstOver) { worstOver = over; defZone = z; needStaff = need; }
        }
    }
    if (hqOver > 0) { defZone = myHQ; needStaff = hqNeed; worstOver = max(worstOver, hqOver); }
    if (hqSiegeMemory) {
        defZone = myHQ;
        needStaff = max(needStaff, min(myArmy, max(4, eField + 2)));
        worstOver = max(worstOver, 1);
    }
    bool recentNarrowBaseSiege =
        !hqSiegeMemory && hqOver <= 0 && !haMode &&
        day >= 70 && day <= 140 && N >= 70 && N <= 85 && nBases <= 3 &&
        lastMyBaseSiegedZone >= 0 &&
        day - lastMyBaseSiegedTurn <= 3 &&
        bld.count(lastMyBaseSiegedZone) &&
        bld[lastMyBaseSiegedZone].owner == meC &&
        !bld[lastMyBaseSiegedZone].hq;
    if (recentNarrowBaseSiege) {
        int z = lastMyBaseSiegedZone;
        defZone = z;
        needStaff = max(needStaff, min(myArmy, max(3, oppInZone(z) + adjEnemyField(z) + 1)));
        worstOver = max(worstOver, 1);
    }
    if (lateHqScreen) {
        defZone = myHQ;
        int screenNeed = lateHqRingLock ? max(10, hqRingThreat + 5 - hqTur)
                                        : max(8, hqOuterThreat + 2);
        needStaff = max(needStaff, min(myArmy, screenNeed));
        worstOver = max(worstOver, 1);
    }
    if (haPathIntercept && hqOver <= 0) {
        defZone = spearZone;
        needStaff = max(needStaff, min(myArmy, max(3, min(eField + 1, hqRingThreat + 2))));
        worstOver = max(worstOver, 1);
    } else if (haMode && spearHop <= 2) {
        defZone = myHQ;
        needStaff = max(needStaff, min(myArmy, max(4, eField - hqTur + 1)));
        worstOver = max(worstOver, 1);
    }

    auto shouldRefillBase = [&](int z, const Bld& b) {
        if (b.hq) return true;
        if (lateHqScreen) return false;
        if (haMode) return z == defZone || z == rallyZone || multiHold.count(z) || (day < 65 && nBases <= 2);
        if (recentNarrowBaseSiege && z != defZone && !multiHold.count(z)) return false;
        if (z == defZone || z == rallyZone || multiHold.count(z)) return true;
        if (b.level >= 2) return true;
        if (day < 70 || nBases <= 3) return true;
        if (eField == 0 && !finishMode && !productionMode && !menace) return true;
        return false;
    };

    map<int,int> kept;
    vector<string> freeW;
    for (auto& kv : war) {
        if (kv.second.owner != meC || kv.second.moving) continue;
        int z = kv.second.zone;
        if (z == myHQ) {
            int lim = max(1, HQ_LAB[hl]);
            if (hqOver > 0) lim = max(lim, hqNeed);
            if (kept[z] < lim) { kept[z]++; continue; }
        } else if (bld.count(z) && bld[z].owner == meC) {
            int lim = bldLabCap(bld[z]);
            if (z == defZone && needStaff > 0) lim = max(lim, needStaff);
            else if (multiHold.count(z)) lim = max(lim, multiHold[z]);
            else if (haMode && z != rallyZone) lim = min(lim, 1);
            if (kept[z] < lim) { kept[z]++; continue; }
        } else if (isBase[z]) {
            bool enemyBldHere = bld.count(z) && bld[z].owner == oppC;
            bool winning = myInZone(z) > oppInZone(z);

            if (enemyBldHere || (oppInZone(z) > 0 && winning)) { kept[z]++; continue; }

            if (!lateIncomeRecovery && !lateAllClearChip &&
                oppInZone(z) == 0 && !bld.count(z) && !(earlyRush && nBases >= 1) &&
                !(raidPressure && nBases >= 2) &&
                !(haMode && nBases >= (compactMap ? 1 : 2)) &&
                !(razeCnt[z] >= 2 && !dominate) &&
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

    bool hqKeeperMode = day >= 150 && oppInZone(myHQ) == 0 && !defenseAlert && !lateAllClearChip;
    if (hqKeeperMode && myInZone(myHQ) == 0 && fi < freeW.size()) {
        size_t best = fi; double bd = 1e18;
        for (size_t k = fi; k < freeW.size(); k++) {
            double d = dist[war[freeW[k]].zone][myHQ];
            if (d < bd) { bd = d; best = k; }
        }
        swap(freeW[fi], freeW[best]);
        if (war[freeW[fi]].zone != myHQ) sendTo(freeW[fi], myHQ);
        fi++;
    }

    for (auto& kv : bld) {
        if (fi >= freeW.size()) break;
        if (kv.second.owner != meC || kv.second.hq) continue;
        int z = kv.first;

        if (dist[myHQ][z] > dist[oppHQ][z]) continue;
        if (!shouldRefillBase(z, kv.second)) continue;
        if (oppInZone(z) > 0 || staffOf(z) >= bldLabCap(kv.second)) continue;
        sendTo(freeW[fi++], z);
    }

    if (needStaff > 0) {
        int have = staffOf(defZone);
        while (fi < freeW.size() && have < needStaff) {
            size_t best = fi; double bd = 1e18;
            for (size_t k = fi; k < freeW.size(); k++) {
                double d = dist[war[freeW[k]].zone][defZone];
                if (d < bd) { bd = d; best = k; }
            }
            swap(freeW[fi], freeW[best]);
            if (war[freeW[fi]].zone == defZone) { fi++; have = staffOf(defZone); continue; }
            if (sendTo(freeW[fi], defZone)) have++;
            fi++;
        }
        if (have < needStaff) {
            vector<pair<double,string>> rec;
            for (auto& kv : war) {
                if (kv.second.owner != meC || kv.second.moving) continue;
                int z = kv.second.zone;
                if (z == defZone || z < 0 || z >= N) continue;
                bool ourHalf = dist[myHQ][z] <= dist[oppHQ][z];
                if (ourHalf && defZone != myHQ && worstOver < 3) continue;
                rec.push_back({dist[z][defZone], kv.first});
            }
            sort(rec.begin(), rec.end());
            for (auto& pr : rec) {
                if (have >= needStaff) break;
                if (sendTo(pr.second, defZone)) have++;
            }
        }
    }

    for (int z : strongOrder) {
        if (fi >= freeW.size() || avail < 10) break;
        if (lateAllClearChip) break;
        if (earlyRush && nBases >= 1) break;
        if (raidPressure && nBases >= 2) break;
        if (haMode && nBases >= (compactMap ? 1 : 2) && !dominate) break;
        if (bld.count(z)) continue;
        if (razeCnt[z] >= 2 && !dominate) continue;
        if (dist[myHQ][z] > dist[oppHQ][z] * EXPAND_REACH) continue;
        if (oppInZone(z) > 0) continue;
        if (staffOf(z) >= 1) continue;
        sendTo(freeW[fi++], z);
    }

    if (fi < freeW.size()) {

        double nearestEnemyD = 1e18;
        for (auto& kv : bld)
            if (kv.second.owner == oppC && dist[myHQ][kv.first] < nearestEnemyD)
                nearestEnemyD = dist[myHQ][kv.first];
        int tgt = -1; double tgtD = 1e18;
        int oppHqDefense = 1000000000;
        bool raceFinishEval = false;
        int stagingTgt = -1, stagingDef = 0;
        bool stagingStrike = false;
        if (bld.count(oppHQ) && bld[oppHQ].owner == oppC && reachable(myHQ, oppHQ)) {
            int ol = bld[oppHQ].level;
            oppHqDefense = oppInZone(oppHQ) + adjEnemyField(oppHQ) + HQ_TUR[ol] + 2;
            int raceAttack = staffOf(oppHQ) + (int)(freeW.size() - fi);
            bool homeNotFlooded = oppInZone(myHQ) <= 2 || myArmy >= eArmyTotal + 4 || day >= 180;
            raceFinishEval = day >= 150 && homeNotFlooded &&
                             (myArmy >= eArmyTotal + 2 || nBases >= oppBases + 1 || day >= 175) &&
                             raceAttack >= max(5, oppHqDefense);
        }
        if (!raceFinishEval && !finishAllIn && !hqChipMode &&
            day >= 168 && day <= 198 && oppInZone(myHQ) == 0 && !defenseAlert &&
            bld.count(oppHQ) && bld[oppHQ].owner == oppC && reachable(myHQ, oppHQ) &&
            (myArmy + 4 >= eArmyTotal || nBases >= oppBases + 1 || day >= 190)) {
            double span = max(1.0, dist[myHQ][oppHQ]);
            int freeAvail0 = (int)(freeW.size() - fi);
            double bestStage = 1e18;
            for (auto& kv : bld) {
                if (kv.second.owner != oppC || kv.second.hq || !reachable(myHQ, kv.first)) continue;
                int z = kv.first, tl = kv.second.level;
                double md = dist[myHQ][z], od = dist[oppHQ][z];
                bool forwardFort = (md <= span * 0.82) || (md + span * 0.05 < od);
                bool corridorFort = fabs(md + od - span) <= span * 0.30;
                if (!forwardFort && !corridorFort) continue;
                int enemyNear = oppInZone(z) + adjEnemyField(z);
                int def = enemyNear + BS_TUR[tl];
                if (staffOf(z) + freeAvail0 < max(3, def + 1)) continue;
                double ourSide = max(0.0, od - md);
                double score = def * 18.0 + md * 0.08 + fabs(md + od - span) * 0.06
                             - ourSide * 0.07 - tl * 6.0 - staffOf(z) * 4.0;
                if (md <= span * 0.42) score -= 18.0;
                if (score < bestStage) {
                    bestStage = score;
                    stagingTgt = z;
                    stagingDef = def;
                }
            }
        }
        for (auto& kv : bld) {
            if (kv.second.owner != oppC || dist[myHQ][kv.first] >= 1e17) continue;
            int z = kv.first, tl = kv.second.level;
            int tur = kv.second.hq ? HQ_TUR[tl] : BS_TUR[tl];
            int def = oppInZone(z) + adjEnemyField(z) + tur;
            double farPenalty = dist[myHQ][z] > nearestEnemyD * 1.6 ? 80.0 : 0.0;
            double score = def * 12.0 + (dist[myHQ][z] - nearestEnemyD) * 0.10 + farPenalty;
            if (lateScoreMode && !kv.second.hq) score += 55.0 + dist[z][oppHQ] * 0.05;
            if (kv.second.hq && !finishAllIn && !lateScoreMode) score += 45.0;
            if (score < tgtD) { tgtD = score; tgt = z; }
        }

        if (stagingTgt >= 0) {
            tgt = stagingTgt;
            stagingStrike = true;
        }

        if (raceFinishEval)
            tgt = oppHQ;

        if (lateAllClearChip)
            tgt = oppHQ;

        if ((finishAllIn || hqChipMode || lateScoreMode) && bld.count(oppHQ) && bld[oppHQ].owner == oppC && reachable(myHQ, oppHQ))
            tgt = oppHQ;

        int clTgt = -1, clE = 0;
        for (int z = 0; z < N; z++) {
            if (bld.count(z)) continue;
            int e = oppInZone(z);
            if (e > 0 && dist[myHQ][z] <= dist[oppHQ][z] && e > clE) { clE = e; clTgt = z; }
        }

        auto reinforceTo = [&](int dest, int req) {
            int have = staffOf(dest);
            while (fi < freeW.size() && avail >= 10 && have < req) {
                size_t best = fi; double bd = 1e18;
                for (size_t k = fi; k < freeW.size(); k++) {
                    double d = dist[war[freeW[k]].zone][dest];
                    if (d < bd) { bd = d; best = k; }
                }
                swap(freeW[fi], freeW[best]);
                if (war[freeW[fi]].zone == dest) { fi++; continue; }
                if (sendTo(freeW[fi], dest)) have++;
                fi++;
            }
        };

        bool launched = false;

        if (haMode && !launched) {
            int waveTarget = -1;
            double waveScore = 1e18;
            for (auto& kv : bld) {
                if (kv.second.owner != oppC || kv.second.hq || dist[rallyZone][kv.first] >= 1e17) continue;
                int z = kv.first;
                int tl = kv.second.level;
                int def = oppInZone(z) + adjEnemyField(z) + BS_TUR[tl];
                double score = def * 20.0 + dist[rallyZone][z] * 0.45 + dist[myHQ][z] * 0.15;
                if (score < waveScore) { waveScore = score; waveTarget = z; }
            }
            if (waveTarget < 0 && day >= 120 && bld.count(oppHQ) && bld[oppHQ].owner == oppC) waveTarget = oppHQ;
            if (waveTarget >= 0) {
                int tl = bld[waveTarget].level;
                int defT = oppInZone(waveTarget) + adjEnemyField(waveTarget) + (bld[waveTarget].hq ? HQ_TUR[tl] : BS_TUR[tl]);
                int atT = staffOf(waveTarget), freeAvail = (int)(freeW.size() - fi);
                int req = max(5, defT + 1);
                if (waveTarget == oppHQ) req = max(7, defT + 2);
                if (atT + freeAvail >= req || (atT > 0 && atT + freeAvail > defT)) {
                    reinforceTo(waveTarget, atT + freeAvail);
                    launched = true;
                }
            }
        }

        if (!launched && (!menace || closeMode || haMode)) {

            if (!lateScoreMode && !closeMode && !launched && clTgt >= 0) {
                int defC = clE + adjEnemyField(clTgt);
                int req = max((finishMode || dominate) ? 3 : 5, defC + 1 - (finishMode ? 1 : 0));
                int atT = staffOf(clTgt), freeAvail = (int)(freeW.size() - fi);
                bool mustered = (atT + freeAvail) >= req;
                bool pressing = atT > 0 && (atT + freeAvail) > defC;
                bool finishPush = finishMode && (atT + freeAvail) >= max(2, req) && (myArmy + 2 >= eArmyTotal || day >= 175);
                if (dominate || mustered || pressing || finishPush) {
                    reinforceTo(clTgt, (dominate || finishAllIn) ? (atT + freeAvail) : req);
                    launched = true;
                }
            }

            if (!launched && tgt >= 0) {
                int tl = bld[tgt].level;
                int adjE = adjEnemyField(tgt);
                int minWave = bld[tgt].hq ? 7 : ((finishMode || dominate || closeMode) ? 4 : 5);
                int req = max(minWave, oppInZone(tgt) + adjE + tl + (bld[tgt].hq ? 2 : 1));
                if (lateAllClearChip && bld[tgt].hq) req = 1;
                if (raceFinishEval && bld[tgt].hq) req = max(5, min(req, oppHqDefense));
                if (stagingStrike && !bld[tgt].hq) req = max(3, min(req, stagingDef + 1));
                if (finishMode && !bld[tgt].hq) req = max(2, req - 1);
                if (closeMode && !bld[tgt].hq) req = max(2, req - 2);
                if (finishAllIn && bld[tgt].hq) req = max(3, req - 2);
                if (hqChipMode && bld[tgt].hq) req = max(3, req - 3);
                int atT = staffOf(tgt), freeAvail = (int)(freeW.size() - fi);
                int defT = oppInZone(tgt) + adjE + (bld[tgt].hq ? HQ_TUR[tl] : BS_TUR[tl]);
                bool mustered = (atT + freeAvail) >= req;
                bool pressing = atT > 0 && (atT + freeAvail) > defT;
                bool finishPush = finishMode && (atT + freeAvail) >= max(3, req) && (myArmy + 2 >= eArmyTotal || day >= 175);
                bool closePush = closeMode && (atT + freeAvail) >= max(2, req - 1);
                bool hqChipPush = hqChipMode && bld[tgt].hq && (atT + freeAvail) >= max(3, req - 1);
                bool racePush = raceFinishEval && bld[tgt].hq && (atT + freeAvail) >= max(5, req);
                bool stagingPush = stagingStrike && !bld[tgt].hq && (atT + freeAvail) >= max(3, req);
                bool allClearChipPush = lateAllClearChip && bld[tgt].hq && (atT + freeAvail) >= 1;
                if (dominate || mustered || pressing || finishPush || closePush || hqChipPush || racePush || stagingPush || allClearChipPush) {
                    reinforceTo(tgt, (dominate || finishAllIn || closePush || hqChipPush || racePush || stagingPush || allClearChipPush) ? (atT + freeAvail) : req);
                    launched = true;
                }
            }
        }

        int funnelTo = (defZone == myHQ || defenseAlert) ? myHQ : rallyZone;
        if (haMode && defZone != myHQ) funnelTo = rallyZone;
        if (recentNarrowBaseSiege && !defenseAlert) funnelTo = defZone;
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
    lastMyHqSieged = false;
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
        bool hqHere = bld.count(d) && bld[d].hq;
        if (t == meC) {
            if (!bld.count(d) || (bld[d].owner != meC && !hqHere)) bld[d] = {meC, 1, BS_HP[1], false};
            continue;
        }
        if (!bld.count(d) || (bld[d].owner != t && !hqHere)) {
            if (bld.count(d) && bld[d].owner == meC && !bld[d].hq) razeCnt[d]++;
            bld[d] = {t, 1, BS_HP[1], false};
        } else {
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
        if (t == meC && d == myHQ) lastMyHqSieged = true;
        if (t == meC && d != myHQ) {
            lastMyBaseSiegedZone = d;
            lastMyBaseSiegedTurn = T;
        }
        auto it = bld.find(d);
        if (it != bld.end()) it->second.hp -= h;
    }
    for (auto it = war.begin(); it != war.end();) { if (it->second.hp <= 0) it = war.erase(it); else ++it; }
    for (auto it = bld.begin(); it != bld.end();) {
        if (it->second.hp <= 0) {
            if (it->second.owner == meC && !it->second.hq) razeCnt[it->first]++;
            it = bld.erase(it);
        } else ++it;
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
