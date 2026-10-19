#include "ui/dialogs/TrackDialog.h"

#include <QButtonGroup>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

namespace {

QIcon makeSwatchIcon(const QColor& color, bool checked) {
    constexpr int kSize = 24;
    QPixmap pixmap(kSize, kSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Outer circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF(2, 2, kSize - 4, kSize - 4));

    if (checked) {
        // High-contrast visible inner ring and dot
        painter.setPen(QPen(Qt::white, 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(4, 4, kSize - 8, kSize - 8));

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawEllipse(QRectF(kSize / 2.0 - 2.5, kSize / 2.0 - 2.5, 5.0, 5.0));
    }

    return QIcon(pixmap);
}

QIcon makeTrackDotIcon(const QColor& color) {
    constexpr int kSize = 12;
    QPixmap pixmap(kSize, kSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF(1, 1, kSize - 2, kSize - 2));
    return QIcon(pixmap);
}

}  // namespace

const QList<TrackDialog::Swatch>& TrackDialog::presetSwatches() {
    static const QList<Swatch> swatches = {
        {u"#5B6CFF"_s, tr("Indigo")},
        {u"#0D9488"_s, tr("Teal")},
        {u"#F59E0B"_s, tr("Amber")},
        {u"#10B981"_s, tr("Emerald")},
        {u"#F43F5E"_s, tr("Rose")},
        {u"#0284C7"_s, tr("Sky")},
        {u"#8B5CF6"_s, tr("Purple")},
        {u"#F97316"_s, tr("Orange")},
    };
    return swatches;
}

TrackDialog::TrackDialog(Data::TrackRepository& tracks,
                         Data::TopicRepository& topics,
                         QWidget* parent)
    : QDialog(parent), m_tracks(tracks), m_topics(topics) {
    setWindowTitle(tr("Manage Tracks"));
    setModal(true);
    resize(520, 420);
    setupUi();
    populateTracks();
}

void TrackDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // List of existing tracks + New Track button
    auto* listHeaderLayout = new QHBoxLayout();
    auto* listLabel = new QLabel(tr("Tracks:"), this);
    listHeaderLayout->addWidget(listLabel);
    listHeaderLayout->addStretch(1);

    m_newBtn = new QPushButton(tr("+ New Track"), this);
    m_newBtn->setObjectName(u"TrackNewBtn"_s);
    listHeaderLayout->addWidget(m_newBtn);
    mainLayout->addLayout(listHeaderLayout);

    m_trackList = new QListWidget(this);
    m_trackList->setObjectName(u"TrackListWidget"_s);
    m_trackList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_trackList->setMaximumHeight(140);
    mainLayout->addWidget(m_trackList);

    // Form area for editing selected or new track
    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setObjectName(u"TrackNameEdit"_s);
    m_nameEdit->setPlaceholderText(tr("Track name (e.g. C++, English)"));
    formLayout->addRow(tr("Name:"), m_nameEdit);

    // Color Swatches
    auto* swatchContainer = new QWidget(this);
    auto* swatchLayout = new QHBoxLayout(swatchContainer);
    swatchLayout->setContentsMargins(0, 0, 0, 0);
    swatchLayout->setSpacing(6);

    m_swatchGroup = new QButtonGroup(this);
    m_swatchGroup->setExclusive(true);

    const auto& swatches = presetSwatches();
    for (int i = 0; i < swatches.size(); ++i) {
        const auto& sw = swatches[i];
        auto* btn = new QPushButton(swatchContainer);
        btn->setObjectName(u"TrackSwatch_%1"_s.arg(i));
        btn->setCheckable(true);
        btn->setFocusPolicy(Qt::StrongFocus);
        btn->setToolTip(sw.name);
        btn->setFixedSize(32, 32);
        btn->setProperty("colorHex", sw.hex);
        btn->setIcon(makeSwatchIcon(QColor(sw.hex), false));
        btn->setIconSize(QSize(24, 24));

        m_swatchGroup->addButton(btn, i);
        swatchLayout->addWidget(btn);
        m_swatchButtons.append(btn);
    }
    swatchLayout->addStretch(1);
    formLayout->addRow(tr("Color:"), swatchContainer);

    mainLayout->addLayout(formLayout);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"TrackMessageLabel"_s);
    m_messageLabel->setStyleSheet(u"color: #ef4444;"_s);
    m_messageLabel->hide();
    mainLayout->addWidget(m_messageLabel);

    // Bottom action bar
    auto* bottomLayout = new QHBoxLayout();
    m_deleteBtn = new QPushButton(tr("Delete Track"), this);
    m_deleteBtn->setObjectName(u"TrackDeleteBtn"_s);
    m_deleteBtn->setEnabled(false);
    bottomLayout->addWidget(m_deleteBtn);

    bottomLayout->addStretch(1);

    m_saveBtn = new QPushButton(tr("Add Track"), this);
    m_saveBtn->setObjectName(u"TrackSaveBtn"_s);
    m_saveBtn->setProperty("primary", true);
    m_saveBtn->setDefault(true);
    bottomLayout->addWidget(m_saveBtn);

    m_closeBtn = new QPushButton(tr("Close"), this);
    m_closeBtn->setObjectName(u"TrackCloseBtn"_s);
    bottomLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomLayout);

    connect(m_trackList, &QListWidget::currentRowChanged, this, &TrackDialog::onTrackSelectionChanged);
    connect(m_newBtn, &QPushButton::clicked, this, &TrackDialog::onNewTrackClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &TrackDialog::onSaveClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TrackDialog::onDeleteClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &TrackDialog::onNameChanged);
    connect(m_swatchGroup, &QButtonGroup::idToggled, this, &TrackDialog::onSwatchToggled);

    if (!m_swatchButtons.isEmpty()) {
        m_swatchButtons.first()->setChecked(true);
    }
}

void TrackDialog::populateTracks() {
    m_trackList->clear();
    const auto all = m_tracks.all();
    for (const auto& track : all) {
        auto* item = new QListWidgetItem(makeTrackDotIcon(QColor(track.color)), track.name, m_trackList);
        item->setData(Qt::UserRole, track.id);
    }

    if (!all.isEmpty()) {
        m_trackList->setCurrentRow(0);
    } else {
        onNewTrackClicked();
    }
}

void TrackDialog::selectTrack(int trackId) {
    for (int i = 0; i < m_trackList->count(); ++i) {
        if (m_trackList->item(i)->data(Qt::UserRole).toInt() == trackId) {
            m_trackList->setCurrentRow(i);
            return;
        }
    }
}

void TrackDialog::setSelectedSwatchColor(const QString& hex) {
    for (auto* btn : m_swatchButtons) {
        if (btn->property("colorHex").toString().compare(hex, Qt::CaseInsensitive) == 0) {
            btn->setChecked(true);
            return;
        }
    }
    if (!m_swatchButtons.isEmpty()) {
        m_swatchButtons.first()->setChecked(true);
    }
}

QString TrackDialog::selectedSwatchColor() const {
    if (auto* checked = m_swatchGroup->checkedButton()) {
        return checked->property("colorHex").toString();
    }
    return presetSwatches().first().hex;
}

void TrackDialog::onSwatchToggled(int id, bool checked) {
    if (id >= 0 && id < m_swatchButtons.size()) {
        auto* btn = m_swatchButtons[id];
        const QString hex = btn->property("colorHex").toString();
        btn->setIcon(makeSwatchIcon(QColor(hex), checked));
    }
}

void TrackDialog::onTrackSelectionChanged() {
    auto* item = m_trackList->currentItem();
    if (!item) {
        m_selectedTrackId.reset();
        m_deleteBtn->setEnabled(false);
        m_saveBtn->setText(tr("Add Track"));
        return;
    }

    const int trackId = item->data(Qt::UserRole).toInt();
    const auto track = m_tracks.byId(trackId);
    if (!track) return;

    m_selectedTrackId = trackId;
    m_nameEdit->setText(track->name);
    setSelectedSwatchColor(track->color);
    m_deleteBtn->setEnabled(true);
    m_saveBtn->setText(tr("Save Track"));
    validateForm();
}

void TrackDialog::onNewTrackClicked() {
    m_trackList->clearSelection();
    m_trackList->setCurrentItem(nullptr);
    m_selectedTrackId.reset();
    m_nameEdit->clear();
    m_nameEdit->setFocus();
    setSelectedSwatchColor(presetSwatches().first().hex);
    m_deleteBtn->setEnabled(false);
    m_saveBtn->setText(tr("Add Track"));
    validateForm();
}

void TrackDialog::onNameChanged(const QString&) {
    validateForm();
}

bool TrackDialog::validateForm() {
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_messageLabel->setText(tr("Track name cannot be empty."));
        m_messageLabel->show();
        m_saveBtn->setEnabled(false);
        return false;
    }

    const auto existingId = m_tracks.idByName(name);
    if (existingId.has_value() && (!m_selectedTrackId.has_value() || *existingId != *m_selectedTrackId)) {
        m_messageLabel->setText(tr("A track with this name already exists."));
        m_messageLabel->show();
        m_saveBtn->setEnabled(false);
        return false;
    }

    m_messageLabel->hide();
    m_saveBtn->setEnabled(true);
    return true;
}

void TrackDialog::onSaveClicked() {
    if (!validateForm()) return;

    const QString name = m_nameEdit->text().trimmed();
    const QString color = selectedSwatchColor();

    if (m_selectedTrackId.has_value()) {
        auto track = m_tracks.byId(*m_selectedTrackId);
        if (!track) return;
        track->name = name;
        track->color = color;
        if (!m_tracks.update(*track)) {
            m_messageLabel->setText(tr("Could not update track: %1").arg(m_tracks.lastError()));
            m_messageLabel->show();
            return;
        }
        const int currentId = *m_selectedTrackId;
        populateTracks();
        selectTrack(currentId);
    } else {
        Domain::Track track;
        track.name = name;
        track.color = color;
        track.position = m_tracks.count();
        if (!m_tracks.insert(track)) {
            m_messageLabel->setText(tr("Could not add track: %1").arg(m_tracks.lastError()));
            m_messageLabel->show();
            return;
        }
        populateTracks();
        selectTrack(track.id);
    }
}

void TrackDialog::onDeleteClicked() {
    if (!m_selectedTrackId.has_value()) return;

    const int trackId = *m_selectedTrackId;
    const auto track = m_tracks.byId(trackId);
    if (!track) return;

    const int topicCount = static_cast<int>(m_topics.byTrack(trackId).size());

    const auto answer = QMessageBox::question(
        this, tr("Delete Track"),
        tr("Delete track \u201c%1\u201d? This will delete %2 topic(s) and their checklist(s). Logged study time will be kept.")
            .arg(track->name).arg(topicCount),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    if (!m_tracks.remove(trackId)) {
        m_messageLabel->setText(tr("Could not delete track: %1").arg(m_tracks.lastError()));
        m_messageLabel->show();
        return;
    }

    m_selectedTrackId.reset();
    populateTracks();
}

}  // namespace JobPrep::Ui::Dialogs
