#include "BooksimRoutingCatalog.h"
#include <QCoreApplication>
#include <QSet>
#include <QStringList>

namespace {

[[nodiscard]] const QStringList& meshIds() {
    static const QStringList k{
        QStringLiteral("dor"),
        QStringLiteral("dim_order"),
        QStringLiteral("dim_order_ni"),
        QStringLiteral("dim_order_pni"),
        QStringLiteral("xy_yx"),
        QStringLiteral("adaptive_xy_yx"),
        QStringLiteral("romm"),
        QStringLiteral("romm_ni"),
        QStringLiteral("min_adapt"),
        QStringLiteral("planar_adapt"),
        QStringLiteral("valiant"),
        QStringLiteral("chaos"),
    };
    return k;
}

[[nodiscard]] const QStringList& torusIds() {
    static const QStringList k{QStringLiteral("dim_order"),
                               QStringLiteral("dim_order_ni"),
                               QStringLiteral("dim_order_bal"),
                               QStringLiteral("min_adapt"),
                               QStringLiteral("valiant"),
                               QStringLiteral("valiant_ni"),
                               QStringLiteral("chaos")};
    return k;
}

[[nodiscard]] const QStringList& cmeshIds() {
    static const QStringList k{QStringLiteral("dor_no_express"),
                               QStringLiteral("dor"),
                               QStringLiteral("xy_yx_no_express"),
                               QStringLiteral("xy_yx")};
    return k;
}

[[nodiscard]] const QStringList& flatflyIds() {
    static const QStringList k{QStringLiteral("ran_min"),
                               QStringLiteral("xyyx"),
                               QStringLiteral("adaptive_xyyx"),
                               QStringLiteral("valiant"),
                               QStringLiteral("ugal"),
                               QStringLiteral("ugal_pni"),
                               QStringLiteral("ugal_xyyx")};
    return k;
}

} // namespace

QStringList routingIdsForTopology(const QString& topologyId) {
    const QString t = topologyId.trimmed().toLower();
    if (t == QLatin1String("mesh")) {
        return meshIds();
    }
    if (t == QLatin1String("torus")) {
        return torusIds();
    }
    if (t == QLatin1String("cmesh")) {
        return cmeshIds();
    }
    if (t == QLatin1String("fly")) {
        return {QStringLiteral("dest_tag")};
    }
    if (t == QLatin1String("qtree")) {
        return {QStringLiteral("nca")};
    }
    if (t == QLatin1String("tree4")) {
        return {QStringLiteral("nca"), QStringLiteral("anca")};
    }
    if (t == QLatin1String("fattree")) {
        return {QStringLiteral("nca"), QStringLiteral("anca")};
    }
    if (t == QLatin1String("flatfly")) {
        return flatflyIds();
    }
    if (t == QLatin1String("dragonflynew")) {
        return {QStringLiteral("min"), QStringLiteral("ugal")};
    }
    if (t == QLatin1String("anynet")) {
        return {QStringLiteral("min")};
    }
    // 未知拓扑：仅列出常见基名；具体仍以自定义 network / BookSim 注册为准
    return QStringList{QStringLiteral("min"), QStringLiteral("dim_order"), QStringLiteral("dor")};
}

QString routingUiLabel(const QString& topologyId, const QString& routingId) {
    const QString topo = topologyId.trimmed().toLower();
    const QString id = routingId;
    const auto T = [](const char* s) {
        return QCoreApplication::translate("BooksimRoutingCatalog", s);
    };
    if (topo == QLatin1String("mesh")) {
        if (id == QLatin1String("dor")) {
            return T("dor · 维序（默认）");
        }
        if (id == QLatin1String("dim_order")) {
            return T("dim_order · 维序");
        }
        if (id == QLatin1String("dim_order_ni")) {
            return T("dim_order_ni · 维序 NI");
        }
        if (id == QLatin1String("dim_order_pni")) {
            return T("dim_order_pni · 维序 PNI");
        }
        if (id == QLatin1String("xy_yx")) {
            return T("xy_yx · XY/YX");
        }
        if (id == QLatin1String("adaptive_xy_yx")) {
            return T("adaptive_xy_yx · 自适应 XY/YX");
        }
        if (id == QLatin1String("romm")) {
            return T("romm · ROMM");
        }
        if (id == QLatin1String("romm_ni")) {
            return T("romm_ni · ROMM NI");
        }
        if (id == QLatin1String("min_adapt")) {
            return T("min_adapt · 最小自适应");
        }
        if (id == QLatin1String("planar_adapt")) {
            return T("planar_adapt · 平面自适应");
        }
        if (id == QLatin1String("valiant")) {
            return T("valiant · Valiant");
        }
        if (id == QLatin1String("chaos")) {
            return T("chaos · Chaos");
        }
    } else if (topo == QLatin1String("torus")) {
        if (id == QLatin1String("dim_order")) {
            return T("dim_order · 维序（默认）");
        }
        if (id == QLatin1String("dim_order_ni")) {
            return T("dim_order_ni · 维序 NI");
        }
        if (id == QLatin1String("dim_order_bal")) {
            return T("dim_order_bal · 维序均衡");
        }
        if (id == QLatin1String("min_adapt")) {
            return T("min_adapt · 最小自适应");
        }
        if (id == QLatin1String("valiant")) {
            return T("valiant · Valiant");
        }
        if (id == QLatin1String("valiant_ni")) {
            return T("valiant_ni · Valiant NI");
        }
        if (id == QLatin1String("chaos")) {
            return T("chaos · Chaos");
        }
    } else if (topo == QLatin1String("cmesh")) {
        if (id == QLatin1String("dor_no_express")) {
            return T("dor_no_express · 维序无捷径（默认）");
        }
        if (id == QLatin1String("dor")) {
            return T("dor · 维序含捷径");
        }
        if (id == QLatin1String("xy_yx_no_express")) {
            return T("xy_yx_no_express · XY/YX 无捷径");
        }
        if (id == QLatin1String("xy_yx")) {
            return T("xy_yx · XY/YX");
        }
    } else if (topo == QLatin1String("fly")) {
        if (id == QLatin1String("dest_tag")) {
            return T("dest_tag · 蝶形（默认）");
        }
    } else if (topo == QLatin1String("qtree")) {
        if (id == QLatin1String("nca")) {
            return T("nca · 最近公共祖先（默认）");
        }
    } else if (topo == QLatin1String("tree4")) {
        if (id == QLatin1String("nca")) {
            return T("nca · 最近公共祖先（默认）");
        }
        if (id == QLatin1String("anca")) {
            return T("anca · 自适应 NCA");
        }
    } else if (topo == QLatin1String("fattree")) {
        if (id == QLatin1String("nca")) {
            return T("nca · 最近公共祖先（默认）");
        }
        if (id == QLatin1String("anca")) {
            return T("anca · 自适应 NCA");
        }
    } else if (topo == QLatin1String("flatfly")) {
        if (id == QLatin1String("ran_min")) {
            return T("ran_min · 随机最小（默认）");
        }
        if (id == QLatin1String("xyyx")) {
            return T("xyyx · XY/YX");
        }
        if (id == QLatin1String("adaptive_xyyx")) {
            return T("adaptive_xyyx · 自适应");
        }
        if (id == QLatin1String("valiant")) {
            return T("valiant · Valiant");
        }
        if (id == QLatin1String("ugal")) {
            return T("ugal · UGAL");
        }
        if (id == QLatin1String("ugal_pni")) {
            return T("ugal_pni · UGAL PNI");
        }
        if (id == QLatin1String("ugal_xyyx")) {
            return T("ugal_xyyx · UGAL XY/YX");
        }
    } else if (topo == QLatin1String("dragonflynew")) {
        if (id == QLatin1String("min")) {
            return T("min · 最小路由（默认）");
        }
        if (id == QLatin1String("ugal")) {
            return T("ugal · UGAL");
        }
    } else if (topo == QLatin1String("anynet")) {
        if (id == QLatin1String("min")) {
            return T("min · 任意网（默认）");
        }
    }
    return id;
}

QString defaultRoutingIdForTopology(const QString& topologyId) {
    const QString t = topologyId.trimmed().toLower();
    if (t == QLatin1String("mesh")) {
        return QStringLiteral("dor");
    }
    if (t == QLatin1String("torus")) {
        return QStringLiteral("dim_order");
    }
    if (t == QLatin1String("cmesh")) {
        return QStringLiteral("dor_no_express");
    }
    if (t == QLatin1String("fly")) {
        return QStringLiteral("dest_tag");
    }
    if (t == QLatin1String("qtree") || t == QLatin1String("tree4")
        || t == QLatin1String("fattree")) {
        return QStringLiteral("nca");
    }
    if (t == QLatin1String("dragonflynew")) {
        return QStringLiteral("min");
    }
    if (t == QLatin1String("flatfly")) {
        return QStringLiteral("ran_min");
    }
    if (t == QLatin1String("anynet")) {
        return QStringLiteral("min");
    }
    return QStringLiteral("min");
}

bool isRoutingValidForTopology(const QString& routingId, const QString& topologyId) {
    const QString r = routingId.trimmed().toLower();
    if (r.isEmpty()) {
        return false;
    }
    const QStringList ids = routingIdsForTopology(topologyId);
    const QSet<QString> allowed(ids.begin(), ids.end());
    return allowed.contains(r);
}

QString normalizeRoutingForTopology(const QString& routingFunction, const QString& topologyId) {
    const QString topo = topologyId.trimmed().toLower();
    QString rf = routingFunction.trimmed().toLower();
    const QString suffix = QStringLiteral("_") + topo;
    if (rf.endsWith(suffix)) {
        rf.chop(suffix.size());
    }
    if (rf.isEmpty()) {
        rf = defaultRoutingIdForTopology(topo);
    }
    if (topo == QLatin1String("mesh") && rf == QLatin1String("min")) {
        rf = QStringLiteral("dor");
    }
    if (topo == QLatin1String("torus")
        && (rf == QLatin1String("dor") || rf == QLatin1String("dim_order_pni"))) {
        rf = QStringLiteral("dim_order");
    }
    if (!isRoutingValidForTopology(rf, topo)) {
        rf = defaultRoutingIdForTopology(topo);
    }
    return rf;
}
