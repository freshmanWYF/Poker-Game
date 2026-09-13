#include "GameStore.h"
#include "../core/Player.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QDateTime>

GameStore::GameStore() {}

GameStore& GameStore::instance() {
    static GameStore store;
    return store;
}

QString GameStore::dataDir() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/PokerGame";
    QDir().mkpath(dir);
    return dir;
}

QString GameStore::filePath(const QString& filename) const {
    return dataDir() + "/" + filename;
}

QJsonObject GameStore::readJson(const QString& filename) const {
    QFile file(filePath(filename));
    if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object();
}

void GameStore::writeJson(const QString& filename, const QJsonObject& obj) {
    QSaveFile file(filePath(filename));
    if (file.open(QIODevice::WriteOnly)) {
        const auto bytes = QJsonDocument(obj).toJson(QJsonDocument::Indented);
        if (file.write(bytes) == bytes.size()) file.commit();
    }
}

// ==================== 筹码存档 ====================

void GameStore::saveChipState(const QList<Player*>& players, int startingChips) {
    QJsonObject root;
    root["startingChips"] = startingChips;
    QJsonArray arr;
    for (auto p : players) {
        QJsonObject obj;
        obj["name"] = p->getName();
        obj["chips"] = p->getChips();
        obj["isAI"] = p->isAI();
        arr.append(obj);
    }
    root["players"] = arr;
    writeJson("game_save.json", root);
}

int GameStore::loadStartingChips() const {
    const int chips = readJson("game_save.json")["startingChips"].toInt(GameConstants::INITIAL_CHIPS);
    return chips >= GameConstants::MIN_STARTING_CHIPS && chips <= GameConstants::MAX_STARTING_CHIPS
        ? chips : GameConstants::INITIAL_CHIPS;
}

bool GameStore::hasChipSave() const {
    QFile file(filePath("game_save.json"));
    return file.exists();
}

void GameStore::loadChipState(QList<QPair<QString, int>>& outChips) {
    outChips.clear();
    QJsonObject root = readJson("game_save.json");
    QJsonArray arr = root["players"].toArray();
    for (auto v : arr) {
        QJsonObject obj = v.toObject();
        outChips.append({obj["name"].toString(), qMax(0, obj["chips"].toInt())});
    }
}

void GameStore::clearChipSave() {
    QFile::remove(filePath("game_save.json"));
}

// ==================== 对局历史 ====================

void GameStore::addMatchRecord(const MatchRecord& record) {
    QJsonObject root = readJson("match_history.json");
    QJsonArray records = root["records"].toArray();

    QJsonObject recObj;
    recObj["time"] = record.time;
    recObj["winner"] = record.winner;
    recObj["winnerType"] = record.winnerType;
    recObj["pot"] = record.pot;

    QJsonArray playersArr;
    for (const auto& entry : record.players) {
        QJsonObject p;
        p["name"] = entry.name;
        p["chipsBefore"] = entry.chipsBefore;
        p["chipsAfter"] = entry.chipsAfter;
        p["handType"] = entry.handType;
        playersArr.append(p);
    }
    recObj["players"] = playersArr;

    records.append(recObj);

    // 最多保留 50 条
    while (records.size() > 50) {
        records.removeFirst();
    }

    root["records"] = records;
    writeJson("match_history.json", root);
}

QList<MatchRecord> GameStore::getMatchHistory(int limit) const {
    QList<MatchRecord> result;
    QJsonObject root = readJson("match_history.json");
    QJsonArray records = root["records"].toArray();

    int start = qMax(0, records.size() - limit);
    for (int i = records.size() - 1; i >= start; --i) {
        QJsonObject recObj = records[i].toObject();
        MatchRecord rec;
        rec.time = recObj["time"].toString();
        rec.winner = recObj["winner"].toString();
        rec.winnerType = recObj["winnerType"].toString();
        rec.pot = recObj["pot"].toInt();

        QJsonArray playersArr = recObj["players"].toArray();
        for (auto v : playersArr) {
            QJsonObject p = v.toObject();
            MatchRecord::PlayerEntry entry;
            entry.name = p["name"].toString();
            entry.chipsBefore = p["chipsBefore"].toInt();
            entry.chipsAfter = p["chipsAfter"].toInt();
            entry.handType = p["handType"].toString();
            rec.players.append(entry);
        }
        result.append(rec);
    }
    return result;
}

// ==================== 战绩统计 ====================

void GameStore::updateStats(const QString& name, bool won, int chipsDelta, const QString& handType) {
    // 特殊235只能克制豹子，不能按枚举值当作最强牌型。
    const QStringList handOrder{"特殊235", "单张", "对子", "顺子", "金花", "顺金", "豹子"};
    QJsonObject root = readJson("player_stats.json");
    QJsonArray stats = root["stats"].toArray();

    bool found = false;
    for (int i = 0; i < stats.size(); ++i) {
        QJsonObject s = stats[i].toObject();
        if (s["name"].toString() == name) {
            if (won) {
                s["wins"] = s["wins"].toInt() + 1;
                s["totalChipsWon"] = s["totalChipsWon"].toInt() + chipsDelta;
            } else {
                s["losses"] = s["losses"].toInt() + 1;
                s["totalChipsLost"] = s["totalChipsLost"].toInt() + qAbs(chipsDelta);
            }
            // 记录最佳牌型（按 HandType 数值）
            QString currentBest = s["bestHand"].toString();
            if (handOrder.indexOf(handType) > handOrder.indexOf(currentBest)) {
                s["bestHand"] = handType;
            }
            stats[i] = s;
            found = true;
            break;
        }
    }

    if (!found) {
        QJsonObject s;
        s["name"] = name;
        s["wins"] = won ? 1 : 0;
        s["losses"] = won ? 0 : 1;
        s["totalChipsWon"] = won ? chipsDelta : 0;
        s["totalChipsLost"] = won ? 0 : qAbs(chipsDelta);
        s["bestHand"] = handOrder.contains(handType) ? handType : QString();
        stats.append(s);
    }

    root["stats"] = stats;
    writeJson("player_stats.json", root);
}

PlayerStats GameStore::getStats(const QString& name) const {
    PlayerStats ps;
    ps.name = name;
    QJsonObject root = readJson("player_stats.json");
    QJsonArray stats = root["stats"].toArray();
    for (auto v : stats) {
        QJsonObject s = v.toObject();
        if (s["name"].toString() == name) {
            ps.wins = s["wins"].toInt();
            ps.losses = s["losses"].toInt();
            ps.totalChipsWon = s["totalChipsWon"].toInt();
            ps.totalChipsLost = s["totalChipsLost"].toInt();
            ps.bestHand = s["bestHand"].toString();
            break;
        }
    }
    return ps;
}

QList<PlayerStats> GameStore::getAllStats() const {
    QList<PlayerStats> result;
    QJsonObject root = readJson("player_stats.json");
    QJsonArray stats = root["stats"].toArray();
    for (auto v : stats) {
        QJsonObject s = v.toObject();
        PlayerStats ps;
        ps.name = s["name"].toString();
        ps.wins = s["wins"].toInt();
        ps.losses = s["losses"].toInt();
        ps.totalChipsWon = s["totalChipsWon"].toInt();
        ps.totalChipsLost = s["totalChipsLost"].toInt();
        ps.bestHand = s["bestHand"].toString();
        result.append(ps);
    }
    return result;
}
