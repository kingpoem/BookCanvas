#include "RouterConfigDialog.h"
#include <ElaLineEdit.h>
#include <ElaPushButton.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

QMap<QString, QString> RouterConfigDialog::s_defaultConfig = {{"topology", "anynet"},
                                                              {"routing_function", "min"},
                                                              {"network_file", "anynet_file"},
                                                              {"traffic", "uniform"},
                                                              {"use_read_write", "0"},
                                                              {"sample_period", "10000"},
                                                              {"injection_rate", "0.01"},
                                                              {"vc_allocator",
                                                               "separable_input_first"},
                                                              {"sw_allocator",
                                                               "separable_input_first"},
                                                              {"alloc_iters", "1"},
                                                              {"num_vcs", "1"},
                                                              {"vc_buf_size", "3"}};

RouterConfigDialog::RouterConfigDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr) {
    setupUI();
    setWindowTitle("全局路由器配置");
    setMinimumSize(500, 600);
}

void RouterConfigDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    QStringList configKeys = {"topology",
                              "routing_function",
                              "network_file",
                              "traffic",
                              "use_read_write",
                              "sample_period",
                              "injection_rate",
                              "vc_allocator",
                              "sw_allocator",
                              "alloc_iters",
                              "num_vcs",
                              "vc_buf_size"};

    QStringList configLabels = {"拓扑结构 (topology)",
                                "路由函数 (routing_function)",
                                "网络文件 (network_file)",
                                "流量模式 (traffic)",
                                "读写模式 (use_read_write)",
                                "采样周期 (sample_period)",
                                "注入速率 (injection_rate)",
                                "VC分配器 (vc_allocator)",
                                "开关分配器 (sw_allocator)",
                                "分配迭代次数 (alloc_iters)",
                                "虚拟通道数 (num_vcs)",
                                "VC缓冲区大小 (vc_buf_size)"};

    for (int i = 0; i < configKeys.size(); ++i) {
        auto* label = new QLabel(configLabels[i], this);
        auto* edit = new ElaLineEdit(this);

        m_edits[configKeys[i]] = edit;

        auto* itemLayout = new QHBoxLayout();
        itemLayout->addWidget(label);
        itemLayout->addWidget(edit);

        m_mainLayout->addLayout(itemLayout);
    }

    m_mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_saveBtn = new ElaPushButton("保存", this);
    m_cancelBtn = new ElaPushButton("取消", this);

    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addWidget(m_cancelBtn);

    m_mainLayout->addLayout(buttonLayout);

    connect(m_saveBtn, &ElaPushButton::clicked, this, &RouterConfigDialog::onSaveClicked);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &RouterConfigDialog::onCancelClicked);

    initializeDefaultConfig();
}

void RouterConfigDialog::initializeDefaultConfig() {
    for (auto it = s_defaultConfig.begin(); it != s_defaultConfig.end(); ++it) {
        if (m_edits.contains(it.key())) {
            m_edits[it.key()]->setText(it.value());
        }
    }
}

void RouterConfigDialog::setConfig(const QMap<QString, QString>& config) {
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (m_edits.contains(it.key())) {
            m_edits[it.key()]->setText(it.value());
        }
    }
}

QMap<QString, QString> RouterConfigDialog::getConfig() const {
    QMap<QString, QString> config;
    for (auto it = m_edits.begin(); it != m_edits.end(); ++it) {
        config[it.key()] = it.value()->text();
    }
    return config;
}

void RouterConfigDialog::onSaveClicked() {
    accept();
}

void RouterConfigDialog::onCancelClicked() {
    reject();
}
