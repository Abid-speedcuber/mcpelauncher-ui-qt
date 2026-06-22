#pragma once

#include <QDebug>
#include <QDir>
#include <QFile>

inline void applyHighDistanceAssetPatches(QString const& versionDir) {
    auto configPath = versionDir + "/assets/assets/renderer/render_distance_configs/render_distance_configuration.android.json";
    if (QFile::exists(configPath)) {
        QFile config(configPath);
        if (config.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            config.write("{\n"
                         "  \"deferred_render_distance_configuration\": {\n"
                         "    \"file\": \"lods/render_distance_configuration_high.json\"\n"
                         "  }\n"
                         "}\n");
        } else {
            qWarning() << "Could not patch Android render distance profile:" << configPath << config.errorString();
        }
    }

    QDir hbuiDir(versionDir + "/assets/assets/gui/dist/hbui");
    auto const fallback = QByteArrayLiteral("?e:[4,6,8]).map");
    auto const expanded = QByteArrayLiteral("?e:[4,6,8,10,12,14,16,18,20,22]).map");
    for (auto const& filename : hbuiDir.entryList({"*.js"}, QDir::Files)) {
        auto path = hbuiDir.filePath(filename);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        auto contents = file.readAll();
        file.close();

        if (!contents.contains("simulationDistanceOptions") ||
            !contents.contains("simulationDistanceDescriptionOnlyOne") ||
            contents.count(fallback) != 1)
            continue;

        contents.replace(fallback, expanded);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Could not patch OreUI simulation distance fallback:" << path << file.errorString();
            continue;
        }
        file.write(contents);
        qDebug() << "Expanded OreUI simulation distance fallback:" << path;
    }
}
