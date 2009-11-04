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

#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <qhash.h>
#include <qkeysequence.h>
#include <qobject.h>

// Ugly but convenience shortcut access macro
#define SHORTCUTS(x) Shortcuts::shortcutsFor(Shortcuts::x)

class Shortcuts : public QObject
{
    friend class SettingsDialog;
    friend class ShortcutEditor;

    Q_OBJECT

public:
    // The integer values are not important here, since the actions are mapped
    // to strings for saving and loading. However, the first action should be 0
    enum Action {
        NoAction = -1,
        NewWindow = 0,      // File
        NewTab,
        OpenFile,
        OpenLocation,
        CloseTab,
        SaveAs,
        Print,
        PrivateBrowsing,
        CloseWindow,
        Find,               // Edit
        FindNext,
        FindPrevious,
        Preferences,
        ViewToolbar,        // View
        ViewBookmarsBar,
        ViewStatusBar,
        Stop,
        ReloadPage,
        ZoomIn,
        ZoomNormal,
        ZoomOut,
        FullScreen,
        PageSource,
        HistoryBack,        // History
        HistoryForward,
        HistoryHome,
        ShowAllHistory,
        ClearHistory,
        ShowAllBookmarks,   // Bookmarks
        AddBookmark,
        BookmarkAllTabs,
        NextTab,            // Window
        PreviousTab,
        ShowDownloads,
        WebSearch,          // Tools
        ClearPrivateData,
        ShowNetworkMonitor,
        EnableWebInspector,
		AdBlock,
        SwitchAppLanguage,  // Help
        _NumActions
    };

    typedef QMultiHash<Action, QKeySequence> Scheme;

public:
    static QList<QKeySequence> shortcutsFor(Action action);
    static QList<QKeySequence> shortcutsFor(const QString &name);
    static QString shortcutName(Action action);
    static Action shortcutAction(const QString &name);

    static Scheme scheme(const QString &name);
    static QStringList schemes();
    static Scheme currentScheme();
    static QString currentSchemeName();
    static void setCurrentScheme(const QString &name);

    static bool isFactoryScheme(const QString &name);
    static void retranslate();

    static void save();

private:
    static void load();
    static void createDefaultSchemes();
    static void init();
    static QString saveScheme(const QString &name, const Scheme &scheme);
    static void deleteScheme(const QString &name);

public:
    static const QString defaultScheme;

private:
    static bool m_loaded;
    static QHash<QString, Scheme> m_schemes;
    static QString m_currentScheme;
    static QHash<QString, Action> m_nameToAction;
};

#endif // SHORTCUTMANAGER_H
