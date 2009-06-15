/*
 * Copyright 2009 Jonas Gehring <jonas.gehring@boolsoft.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "shortcuteditor.h"

#include "lineedit.h"
#include "clearbutton.h"

#include <qboxlayout.h>
#include <qlineedit.h>
#include <qsignalmapper.h>
#include <qtoolbutton.h>

ShortcutKeySequenceEdit::ShortcutKeySequenceEdit(QWidget *parent)
    : QWidget(parent)
{
    m_lineEdit = new LineEdit(this); 
    ClearButton *clearButton = new ClearButton(this);
    m_lineEdit->addWidget(clearButton, LineEdit::RightSide);

    connect(m_lineEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(sequenceChanged()));
    connect(clearButton, SIGNAL(clicked()), m_lineEdit, SLOT(clear()));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_lineEdit);
    layout->setMargin(0);

    m_lineEdit->installEventFilter(this);
    m_lineEdit->setReadOnly(true);
    m_lineEdit->setFocusProxy(this);
    setFocusPolicy(m_lineEdit->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
}

QKeySequence ShortcutKeySequenceEdit::sequence() const
{
    return m_sequence;
}

void ShortcutKeySequenceEdit::setSequence(const QKeySequence &sequence)
{
    m_sequence = sequence;
    m_lineEdit->setText(sequence.toString());
}


ShortcutDialog::ShortcutDialog(const QString &name, const QList<QKeySequence> &sequences, QWidget *parent)
    : QDialog(parent), m_sequences(sequences), m_removeMapper(new QSignalMapper(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Shortcuts for action <b>%1</b>:").arg(name)));

    for (int i = 0; i < m_sequences.count(); i++)
        layout->addLayout(makeSequenceEdit(i, m_sequences[i]));

    layout->addStretch(1);
    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    connect(buttons->addButton(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttons->addButton(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
    layout->addWidget(buttons);

    connect(m_removeMapper, SIGNAL(mapped(int)), this, SLOT(remove(int)));

    setWindowTitle(tr("Shortcut Editor"));
}

QList<QKeySequence> ShortcutDialog::sequences() const
{
    return m_sequences;
}

void ShortcutDialog::add()
{

}

void ShortcutDialog::remove(int index)
{

}

void ShortcutDialog::changed()
{

}

QHBoxLayout *ShortcutDialog::makeSequenceEdit(int index, const QKeySequence &sequence)
{
    QHBoxLayout *layout = new QHBoxLayout();
    ShortcutKeySequenceEdit *edit = new ShortcutKeySequenceEdit(this);
    edit->setSequence(sequence);
    layout->addWidget(edit);

    QToolButton *addButton = new QToolButton(this);
    addButton->setText(tr("+"));
    layout->addWidget(addButton);
    QToolButton *removeButton = new QToolButton(this);
    removeButton->setText(tr("-"));
    layout->addWidget(removeButton);

    connect(edit, SIGNAL(sequenceChanged()), this, SLOT(changed()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
    m_removeMapper->setMapping(removeButton, index);
    connect(removeButton, SIGNAL(clicked()), m_removeMapper, SLOT(map()));
    
    return layout;
}


ShortcutEditorModel::ShortcutEditorModel(QObject *parent)
    : QAbstractItemModel(parent)
{

}

ShortcutManager::Scheme ShortcutEditorModel::scheme() const
{
    return m_scheme;
}

void ShortcutEditorModel::setScheme(const ShortcutManager::Scheme &scheme)
{
    m_scheme = scheme;
}

ShortcutManager::Action ShortcutEditorModel::action(const QModelIndex &index) const
{
    return (ShortcutManager::Action)index.row();
}

QList<QKeySequence> ShortcutEditorModel::sequences(const QModelIndex &index) const
{
    return m_scheme.values((ShortcutManager::Action)index.row());
}

void ShortcutEditorModel::setSequences(ShortcutManager::Action action, const QList<QKeySequence> &sequences)
{
    m_scheme.remove(action);
    for (int i = 0; i < sequences.count(); i++)
        m_scheme.insert(action, sequences[i]);
}

QVariant ShortcutEditorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return ShortcutManager::shortcutName(action(index));
        } else {
            QList<QKeySequence> shortcuts = sequences(index);
            QString tmp;
            for (int i = 0; i < shortcuts.count(); i++) {
                tmp += shortcuts[i].toString();
                if (i != shortcuts.count()-1) {
                    tmp += QLatin1String(", ");
                }
            }
            return tmp;
        }
    } 

    return QVariant();
}

QVariant ShortcutEditorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return tr("Action");
    case 1:
        return tr("Shortcuts");
    }
    return QVariant();
}

QModelIndex ShortcutEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        return QModelIndex();

    return createIndex(row, column, 0);
}

QModelIndex ShortcutEditorModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int ShortcutEditorModel::rowCount(const QModelIndex &parent) const
{
    return (!parent.isValid() ? ShortcutManager::_NumActions : 0);
}

int ShortcutEditorModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}


ShortcutEditor::ShortcutEditor(const QString &schemeName, QWidget *parent)
    : QDialog(parent), m_schemeName(schemeName)
{
    setupUi(this);

    m_model = new ShortcutEditorModel(this);
    if (m_schemeName.isEmpty())
        m_schemeName = QLatin1String("Default");
    m_model->setScheme(ShortcutManager::scheme(m_schemeName));
    shortcuts->setModel(m_model);

    connect(shortcuts, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(edit(const QModelIndex &)));
}

void ShortcutEditor::edit(const QModelIndex &index)
{
    ShortcutManager::Action action = m_model->action(index);
    ShortcutDialog dialog(ShortcutManager::shortcutName(action), m_model->sequences(index));
    if (dialog.exec())
        m_model->setSequences(action, dialog.sequences());
}
