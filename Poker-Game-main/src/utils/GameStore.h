#ifndef GAMESTORE_H
#define GAMESTORE_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

class Player;

// 对局记录
struct MatchRecord {
    QString time;
    QString winner;
    QString winnerType;
    int pot = 0;
    struct PlayerEntry {
        QString name;
        int chipsBefore = 0;
        int chipsAfter = 0;
        QString handType;
    };
    QList<PlayerEntry> players;
};

// 玩家战绩
struct PlayerStats {
    QString name;
    int wins = 0;
    int losses = 0;
    int totalChipsWon = 0;
    int totalChipsLost = 0;
    QString bestHand;
};

/**
 * @brief 持久化管理器（单例）
 * 管理筹码存档、对局历史、战绩统计，使用本地 JSON 文件
 */
class GameStore {
public:
    static GameStore& instance();

    // 筹码存档
    void saveChipState(const QList<Player*>& players);
    bool hasChipSave() const;
    void loadChipState(QList<QPair<QString, int>>& outChips); // <name, chips>
    void clearChipSave();

    // 对局历史
    void addMatchRecord(const MatchRecord& record);
    QList<MatchRecord> getMatchHistory(int limit = 20) const;

    // 战绩统计
    void updateStats(const QString& name, bool won, int chipsDelta, const QString& handType);
    PlayerStats getStats(const QString& name) const;
    QList<PlayerStats> getAllStats() const;

private:
    GameStore();
    QString dataDir() const;
    QString filePath(const QString& filename) const;
    QJsonObject readJson(const QString& filename) const;
    void writeJson(const QString& filename, const QJsonObject& obj);
};

#endif // GAMESTORE_H
