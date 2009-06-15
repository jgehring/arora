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

#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <qhash.h>
#include <qkeysequence.h>
#include <qobject.h>

class ShortcutManager : public QObject
{
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
        CloseWindow,        // Edit
        Find,
        FindNext,
        FindPrevious,
        Preferences,
        HideToolbar,        // View
        ShowBookmarsBar,
        HideStatusBar,
        Stop,
        ReloadPage,
        ZoomIn,
        ZoomNormal,
        ZoomOut,
        FullScreen,
        PageSource,
        HistoryBackward,    // History
        HistoryForward,
        HistoryHome,
        ShowAllHistory,
        ClearHistory,
        ShowAllBookmarsk,   // Bookmarks
        AddBookmark,
        BookmarkAllTabs,
        NextTab,            // Window
        PreviousTab,
        ShowDownloads,
        WebSearch,          // Tools
        ClearPrivateData,
        ShowNetworkMonitor,
        EnableWebInspector,
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
    static QString setScheme(const QString &name, const Scheme &scheme);
    static Scheme currentScheme();
    static QString currentSchemeName();

    static void save();

private:
    static void load();
    static void createDefaultScheme();
    static void init();

    static bool m_loaded;
    static QHash<QString, Scheme > m_schemes;
    static QString m_currentScheme;
    static QHash<QString, Action> m_nameToAction;
};

#endif // SHORTCUTMANAGER_H
