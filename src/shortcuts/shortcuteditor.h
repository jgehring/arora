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

#ifndef SHORTCUTEDITOR_H
#define SHORTCUTEDITOR_H

#include "shortcuts.h"

#include <qabstractitemmodel.h>
#include <qboxlayout.h>
#include <qdialog.h>
#include <qlist.h>
#include "ui_shortcuteditor.h"

class LineEdit;

// This class is loosely based on QtKeySequenceEdit from QtPropertyBrowser
class ShortcutKeySequenceEdit : public QWidget
{
    Q_OBJECT

signals:
    void sequenceChanged();

public:
    ShortcutKeySequenceEdit(QWidget *parent = 0);

    QKeySequence sequence() const;
    void setSequence(const QKeySequence &sequence);

    bool eventFilter(QObject *object, QEvent *event);

protected:
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);

private:
    int translateModifiers(Qt::KeyboardModifiers modifiers, const QString &text) const;

    LineEdit *m_lineEdit;
    QKeySequence m_sequence;
};

class ShortcutKeySequenceEditContainer : public QHBoxLayout
{
    Q_OBJECT

signals:
    void add();
    void remove();

public:
    ShortcutKeySequenceEditContainer(const QKeySequence &sequence, QWidget *parent = 0);

    QKeySequence sequence() const;

private:
    ShortcutKeySequenceEdit *m_edit;
};

class ShortcutDialog : public QDialog
{
    Q_OBJECT

public:
    ShortcutDialog(const QString &name, const QList<QKeySequence> &sequences, QWidget *parent = 0);

    QList<QKeySequence> sequences() const;

private slots:
    void add();
    void remove();

private:
    ShortcutKeySequenceEditContainer *makeContainer(const QKeySequence &sequence);

    QVBoxLayout *m_layout;
}; 

class ShortcutEditorModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ShortcutEditorModel(QObject *parent = 0);

    Shortcuts::Scheme scheme() const;
    void setScheme(const Shortcuts::Scheme &scheme);

    Shortcuts::Action action(const QModelIndex &index) const;
    QList<QKeySequence> sequences(const QModelIndex &index) const;
    void setSequences(Shortcuts::Action action, const QList<QKeySequence> &sequences);

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    Shortcuts::Scheme m_scheme;
};

class ShortcutEditor : public QDialog, public Ui_ShortcutEditor
{
    Q_OBJECT

public:
    ShortcutEditor(const QString &schemeName = QString(), QWidget *parent = 0);

private slots:
    void edit(const QModelIndex &index);

private:
    QString m_schemeName;
    ShortcutEditorModel *m_model;
};

#endif // SHORTCUTEDITOR_H
