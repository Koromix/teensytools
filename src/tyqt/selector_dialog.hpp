/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#ifndef SELECTOR_DIALOG_HH
#define SELECTOR_DIALOG_HH

#include <QIdentityProxyModel>

#include <memory>

#include "ui_selector_dialog.h"

class Board;
class Monitor;

class SelectorDialogModelFilter: public QIdentityProxyModel {
    Q_OBJECT

public:
    SelectorDialogModelFilter(QObject *parent = nullptr)
        : QIdentityProxyModel(parent) {}

    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

class SelectorDialog : public QDialog, private Ui::SelectorDialog {
    Q_OBJECT

    Monitor *monitor_;
    SelectorDialogModelFilter monitor_model_;
    QString action_;

    std::shared_ptr<Board> current_board_;

public:
    SelectorDialog(QWidget *parent = nullptr);

    void setAction(const QString &action);
    QString action() const { return action_; }

    void setDescription(const QString &desc) { descriptionLabel->setText(desc); }
    QString description() const { return descriptionLabel->text(); }

    std::shared_ptr<Board> currentBoard() const { return current_board_; }
    std::shared_ptr<Board> selectedBoard() const;

protected slots:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &previous);
    void done(int result) override;

signals:
    void currentChanged(Board *board);
    void boardSelected(Board *board);
};

#endif
