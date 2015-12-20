/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#include <QPushButton>

#include "selector_dialog.hh"

using namespace std;

SelectorDialog::SelectorDialog(Manager *manager, QWidget *parent)
    : QDialog(parent), manager_(manager)
{
    setupUi(this);

    tree->setModel(manager);
    connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SelectorDialog::selectionChanged);
    connect(tree, &QTreeView::doubleClicked, this, &SelectorDialog::doubleClicked);

    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    current_board_ = manager->board(0);
    if (current_board_) {
        tree->setCurrentIndex(manager->index(0, 0));
    } else {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

shared_ptr<Board> SelectorDialog::selectedBoard() const
{
    return result() ? current_board_ : nullptr;
}

void SelectorDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &previous)
{
    TY_UNUSED(previous);

    if (!selected.indexes().isEmpty()) {
        current_board_ = manager_->board(selected.indexes().front().row());
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        current_board_ = nullptr;
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }

    emit currentChanged(current_board_.get());
}

void SelectorDialog::doubleClicked(const QModelIndex &index)
{
    TY_UNUSED(index);
    accept();
}

void SelectorDialog::done(int result)
{
    QDialog::done(result);
    emit boardSelected(result ? current_board_.get() : nullptr);
}

shared_ptr<Board> SelectorDialog::getBoard(Manager *manager, QWidget *parent)
{
    SelectorDialog dialog(manager, parent);

    dialog.exec();

    return dialog.selectedBoard();
}
