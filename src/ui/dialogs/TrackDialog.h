#pragma once

#include <optional>
#include <QDialog>
#include <QList>
#include <QString>
#include "domain/Structs.h"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace JobPrep::Data {
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Ui::Dialogs {

/// Track management dialog (SPEC §3.2 S1, S9, §5 `tracks`):
/// Add, rename, recolor via preset swatches, and delete with cascade confirmation.
class TrackDialog : public QDialog {
    Q_OBJECT

public:
    struct Swatch {
        QString hex;
        QString name;
    };

    static const QList<Swatch>& presetSwatches();

    TrackDialog(Data::TrackRepository& tracks,
                Data::TopicRepository& topics,
                QWidget* parent = nullptr);
    ~TrackDialog() override = default;

    /// Selects a track by id in the list.
    void selectTrack(int trackId);

private slots:
    void onTrackSelectionChanged();
    void onNewTrackClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onNameChanged(const QString& text);
    void onSwatchToggled(int id, bool checked);

private:
    void setupUi();
    void populateTracks();
    void setSelectedSwatchColor(const QString& hex);
    QString selectedSwatchColor() const;
    bool validateForm();

    Data::TrackRepository& m_tracks;
    Data::TopicRepository& m_topics;

    QListWidget* m_trackList{nullptr};
    QLineEdit* m_nameEdit{nullptr};
    QButtonGroup* m_swatchGroup{nullptr};
    QList<QPushButton*> m_swatchButtons;
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_newBtn{nullptr};
    QPushButton* m_closeBtn{nullptr};

    std::optional<int> m_selectedTrackId;
};

}  // namespace JobPrep::Ui::Dialogs
