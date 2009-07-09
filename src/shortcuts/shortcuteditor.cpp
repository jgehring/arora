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

#include <qevent.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qtoolbutton.h>

ShortcutKeySequenceEdit::ShortcutKeySequenceEdit(QWidget *parent)
    : QWidget(parent), m_lineEdit(new LineEdit(this))
{
    ClearButton *clearButton = new ClearButton(m_lineEdit);
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

bool ShortcutKeySequenceEdit::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_lineEdit) {
        // Fetch key presses using eventFilter() in order to fetch the Tab key
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            keyPressEvent(keyEvent);
            return true;
        } else {
            return false;
        }
    } else{
        return QWidget::eventFilter(object, event);
    }
}

void ShortcutKeySequenceEdit::focusInEvent(QFocusEvent *event)
{
    QCoreApplication::sendEvent(m_lineEdit, event);
    m_lineEdit->grabKeyboard();
    m_lineEdit->selectAll();
    QWidget::focusInEvent(event);
}

void ShortcutKeySequenceEdit::focusOutEvent(QFocusEvent *event)
{
    QCoreApplication::sendEvent(m_lineEdit, event);
    m_lineEdit->releaseKeyboard();
    QWidget::focusOutEvent(event);
}

void ShortcutKeySequenceEdit::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Meta ||
            key == Qt::Key_Alt || key == Qt::Key_Super_L || key == Qt::Key_AltGr)
        return;

    key |= translateModifiers(event->modifiers(), event->text());
    m_sequence = QKeySequence(key);
    m_lineEdit->setText(m_sequence.toString());
    event->accept();
}

int ShortcutKeySequenceEdit::translateModifiers(Qt::KeyboardModifiers modifiers, const QString &text) const
{
    int result = 0;
    if ((modifiers & Qt::ShiftModifier) && (text.size() == 0 || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (modifiers & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (modifiers & Qt::MetaModifier)
        result |= Qt::META;
    if (modifiers & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}


ShortcutKeySequenceEditContainer::ShortcutKeySequenceEditContainer(const QKeySequence &sequence, QWidget *parent)
        : QHBoxLayout(NULL), m_edit(new ShortcutKeySequenceEdit(parent))
{
    m_edit->setSequence(sequence);
    addWidget(m_edit);

    QToolButton *addButton = new QToolButton(parent);
    addButton->setText(tr("+"));
    addWidget(addButton);
    QToolButton *removeButton = new QToolButton(parent);
    removeButton->setText(tr("-"));
    addWidget(removeButton);

    connect(addButton, SIGNAL(clicked()), this, SIGNAL(add()));
    connect(removeButton, SIGNAL(clicked()), this, SIGNAL(remove()));
}

QKeySequence ShortcutKeySequenceEditContainer::sequence() const
{
    return m_edit->sequence();
}


ShortcutDialog::ShortcutDialog(const QString &name, const QList<QKeySequence> &sequences, QWidget *parent)
    : QDialog(parent)
{
    // The layout items before and after the sequence edit widgets are
    // essential, i.e. they are expected to be present in add() and remove()
    m_layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Shortcuts for action <b>%1</b>:").arg(name));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_layout->addWidget(label);

    for (int i = 0; i < sequences.count(); ++i)
        m_layout->addLayout(makeContainer(sequences[i]));
    if (sequences.isEmpty())
        m_layout->addLayout(makeContainer(QKeySequence()));

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    connect(buttons->addButton(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttons->addButton(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
    m_layout->addWidget(buttons);

    setWindowTitle(tr("Shortcut Editor"));
}

QList<QKeySequence> ShortcutDialog::sequences() const
{
    QList<QKeySequence> list;
    for (int i = 1; i < m_layout->count()-1; ++i) {
        QLayout *layout = m_layout->itemAt(i)->layout();
        if (layout == NULL)
            continue;
        ShortcutKeySequenceEditContainer *container = qobject_cast<ShortcutKeySequenceEditContainer*>(layout);
        if (container == NULL)
            continue;
        list.append(container->sequence());
    }
    return list;
}

void ShortcutDialog::add()
{
    m_layout->insertLayout(m_layout->count()-1, makeContainer(QKeySequence()));
}

void ShortcutDialog::remove()
{
    ShortcutKeySequenceEditContainer *container = qobject_cast<ShortcutKeySequenceEditContainer*>(sender());
    if (container == NULL)
        return;

    m_layout->removeItem(container);
    while (container->count() > 0) {
        QWidget *w = container->takeAt(0)->widget();
        if (w != NULL) {
            w->hide();
            w->deleteLater();
        }
    }
    container->deleteLater();

    // Keep a single editor if neccessary
    if (m_layout->count() < 3) {
        add();
    }
    m_layout->activate();
    adjustSize();
}

ShortcutKeySequenceEditContainer *ShortcutDialog::makeContainer(const QKeySequence &sequence)
{
    ShortcutKeySequenceEditContainer *container = new ShortcutKeySequenceEditContainer(sequence, this);
    connect(container, SIGNAL(add()), this, SLOT(add()));
    connect(container, SIGNAL(remove()), this, SLOT(remove()));
    return container;
}


ShortcutEditorModel::ShortcutEditorModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    for (int i = 0; i < Shortcuts::_NumActions; ++i) {
        m_actions.append((Shortcuts::Action)i);
    }
}

Shortcuts::Scheme ShortcutEditorModel::scheme() const
{
    return m_scheme;
}

void ShortcutEditorModel::setScheme(const Shortcuts::Scheme &scheme)
{
    m_scheme = scheme;
    reset();
}

Shortcuts::Action ShortcutEditorModel::action(const QModelIndex &index) const
{
    return m_actions[index.row()];
}

QList<QKeySequence> ShortcutEditorModel::sequences(const QModelIndex &index) const
{
    return m_scheme.values(m_actions[index.row()]);
}

void ShortcutEditorModel::setSequences(Shortcuts::Action action, const QList<QKeySequence> &sequences)
{
    m_scheme.remove(action);
    for (int i = 0; i < sequences.count(); ++i)
        m_scheme.insert(action, sequences[i]);
}

QVariant ShortcutEditorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return Shortcuts::shortcutName(action(index));
        } else {
            QList<QKeySequence> shortcuts = sequences(index);
            QString tmp;
            for (int i = 0; i < shortcuts.count(); ++i) {
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
    Q_UNUSED(index);
    return QModelIndex();
}

int ShortcutEditorModel::rowCount(const QModelIndex &parent) const
{
    return (!parent.isValid() ? Shortcuts::_NumActions : 0);
}

int ShortcutEditorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

static bool shortcutNameLessThan(const Shortcuts::Action &a, const Shortcuts::Action &b)
{
    return Shortcuts::shortcutName(a) < Shortcuts::shortcutName(b);
}

static bool shortcutNameGreaterThan(const Shortcuts::Action &a, const Shortcuts::Action &b)
{
    return Shortcuts::shortcutName(a) > Shortcuts::shortcutName(b);
}

void ShortcutEditorModel::sort(int column, Qt::SortOrder order)
{
    // Sorting by the shortcuts themselves doesn't make much sense
    if (column != 0)
        return;

    if (order == Qt::AscendingOrder)
        qStableSort(m_actions.begin(), m_actions.end(), shortcutNameLessThan);
    else
        qStableSort(m_actions.begin(), m_actions.end(), shortcutNameGreaterThan);
    emit layoutChanged();
}


ShortcutEditor::ShortcutEditor(const QString &schemeName, QWidget *parent)
    : QDialog(parent), m_schemeName(schemeName)
{
    setupUi(this);

    schemeComboBox->addItems(Shortcuts::schemes());
    schemeComboBox->setCurrentIndex(schemeComboBox->findText(Shortcuts::currentSchemeName()));

    m_model = new ShortcutEditorModel(this);
    if (m_schemeName.isEmpty())
        m_schemeName = QLatin1String("Default");
    m_model->setScheme(Shortcuts::scheme(m_schemeName));
    shortcuts->setModel(m_model);

    connect(shortcuts, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(edit(const QModelIndex &)));
    connect(schemeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(selectScheme(const QString &)));
    connect(this, SIGNAL(accepted()), this, SLOT(saveScheme()));
    connect(schemeSaveAsButton, SIGNAL(clicked()), this, SLOT(saveSchemeAs()));
    connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(resetScheme()));
}

void ShortcutEditor::edit(const QModelIndex &index)
{
    Shortcuts::Action action = m_model->action(index);
    ShortcutDialog dialog(Shortcuts::shortcutName(action), m_model->sequences(index));
    if (dialog.exec())
        m_model->setSequences(action, dialog.sequences());
}

void ShortcutEditor::selectScheme(const QString &schemeName)
{
    if (Shortcuts::scheme(m_schemeName) != m_model->scheme()) {
        switch (QMessageBox::question(this, tr("Shortcut editor"), tr("The current scheme has been modified. Do you want to save it?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel)) {
        case QMessageBox::Save:
            saveScheme();
        case QMessageBox::Discard:
            break;
        default:
            // An additional signal for this would be great
            disconnect(schemeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(selectScheme(const QString &)));
            schemeComboBox->setCurrentIndex(schemeComboBox->findText(m_schemeName));
            connect(schemeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(selectScheme(const QString &)));
            return;
        }
    }

    m_schemeName = schemeName;
    m_model->setScheme(Shortcuts::scheme(m_schemeName));
}

void ShortcutEditor::saveScheme()
{
    if (Shortcuts::scheme(m_schemeName) != m_model->scheme())
        Shortcuts::saveScheme(m_schemeName, m_model->scheme());
}

void ShortcutEditor::saveSchemeAs()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save current scheme as..."), tr("Scheme name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        name = Shortcuts::saveScheme(name, m_model->scheme());

        disconnect(schemeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(selectScheme(const QString &)));
        schemeComboBox->clear();
        schemeComboBox->addItems(Shortcuts::schemes());
        schemeComboBox->setCurrentIndex(schemeComboBox->findText(name));
        connect(schemeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(selectScheme(const QString &)));

        m_schemeName = name;
    }
}

void ShortcutEditor::resetScheme()
{
    m_model->setScheme(Shortcuts::scheme(m_schemeName));
}
