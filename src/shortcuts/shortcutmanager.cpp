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

#include "shortcutmanager.h"

#include <qcoreapplication.h>
#include <qdesktopservices.h>
#include <qstringlist.h>
#include <qsettings.h>

bool ShortcutManager::m_loaded = false;
QHash<QString, QMultiHash<ShortcutManager::Action, QKeySequence> > ShortcutManager::m_schemes;
QString ShortcutManager::m_currentScheme = QLatin1String("Default");
QHash<QString, ShortcutManager::Action> ShortcutManager::m_nameToAction;


QList<QKeySequence> ShortcutManager::shortcutsFor(Action action)
{
    load();
    return m_schemes.value(m_currentScheme).values(action);
}

QList<QKeySequence> ShortcutManager::shortcutsFor(const QString &name)
{
    load();
    return shortcutsFor(shortcutAction(name));
}

QString ShortcutManager::shortcutName(Action action)
{
    // Forbidden characters: ':'
    switch (action) {
    case NewWindow:             return QLatin1String("NewWindow");
    case NewTab:                return QLatin1String("NewTab");
    case OpenFile:              return QLatin1String("OpenFile");
    case OpenLocation:          return QLatin1String("OpenLocation");
    case CloseTab:              return QLatin1String("CloseTab");
    case SaveAs:                return QLatin1String("SaveAs");
    case Print:                 return QLatin1String("Print");
    case PrivateBrowsing:       return QLatin1String("PrivateBrowsing");
    case CloseWindow:           return QLatin1String("CloseWindow");
    case Find:                  return QLatin1String("Find");
    case FindNext:              return QLatin1String("FindNext");
    case FindPrevious:          return QLatin1String("FindPrevious");
    case Preferences:           return QLatin1String("Preferences");
    case HideToolbar:           return QLatin1String("HideToolbar");
    case ShowBookmarsBar:       return QLatin1String("ShowBookmarsBar");
    case HideStatusBar:         return QLatin1String("HideStatusBar");
    case Stop:                  return QLatin1String("Stop");
    case ReloadPage:            return QLatin1String("ReloadPage");
    case ZoomIn:                return QLatin1String("ZoomIn");
    case ZoomNormal:            return QLatin1String("ZoomNormal");
    case ZoomOut:               return QLatin1String("ZoomOut");
    case FullScreen:            return QLatin1String("FullScreen");
    case PageSource:            return QLatin1String("PageSource");
    case HistoryBackward:       return QLatin1String("HistoryBackward");
    case HistoryForward:        return QLatin1String("HistoryForward");
    case HistoryHome:           return QLatin1String("HistoryHome");
    case ShowAllHistory:        return QLatin1String("ShowAllHistory");
    case ClearHistory:          return QLatin1String("ClearHistory");
    case ShowAllBookmarsk:      return QLatin1String("ShowAllBookmarsk");
    case AddBookmark:           return QLatin1String("AddBookmark");
    case BookmarkAllTabs:       return QLatin1String("BookmarkAllTabs");
    case NextTab:               return QLatin1String("NextTab");
    case PreviousTab:           return QLatin1String("PreviousTab");
    case ShowDownloads:         return QLatin1String("ShowDownloads");
    case WebSearch:             return QLatin1String("WebSearch");
    case ClearPrivateData:      return QLatin1String("ClearPrivateData");
    case ShowNetworkMonitor:    return QLatin1String("ShowNetworkMonitor");
    case EnableWebInspector:    return QLatin1String("EnableWebInspector");
    case SwitchAppLanguage:     return QLatin1String("SwitchAppLanguage");
    default: break;
    }
    return QString();
}

ShortcutManager::Action ShortcutManager::shortcutAction(const QString &name)
{
    init();
    return m_nameToAction.value(name, NoAction);
}

void ShortcutManager::save()
{
    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QString shortcutsFile = dir + QLatin1String("/shortcuts.conf");
    QSettings settings(shortcutsFile, QSettings::IniFormat); 

    settings.clear();
    settings.setValue(QLatin1String("aroraVersion"), QCoreApplication::applicationVersion());
    settings.setValue(QLatin1String("currentScheme"), m_currentScheme);

    QHashIterator<QString, QMultiHash<Action, QKeySequence> > i(m_schemes);
    while (i.hasNext()) {
        i.next();
        settings.beginGroup(i.key());

        // The key for this shortcut constist of "$num:$action", so we are able
        // to store multiple shotcuts per action
        QHashIterator<Action, QKeySequence> j(i.value());
        int k = 0;
        while (j.hasNext()) {
            j.next();
            settings.setValue(QString(QLatin1String("%1:%2")).arg(k).arg(shortcutName(j.key())), j.value());
            ++k;
        }

        settings.endGroup();
    }
}

void ShortcutManager::load()
{
    if (m_loaded)
        return;

    init();

    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QString shortcutsFile = dir + QLatin1String("/shortcuts.conf");
    QSettings settings(shortcutsFile, QSettings::IniFormat); 

    QString version = settings.value(QLatin1String("aroraVersion")).toString();
    m_currentScheme = settings.value(QLatin1String("currentScheme"), QLatin1String("Default")).toString();

    QStringList schemes = settings.childGroups();
    m_schemes.clear();
    for (int i = 0; i < schemes.count(); i++) {
        settings.beginGroup(schemes[i]);

        QMultiHash<Action, QKeySequence> shortcuts;
        QStringList actions = settings.childKeys();
        for (int j = 0; j < actions.count(); j++) {
            // See comment in save() for an explaination
            QStringList tmp = actions[i].split(QLatin1Char(':'));
            if (tmp.count() < 2) {
                continue;
            }
            shortcuts.insert(shortcutAction(tmp[1]), qvariant_cast<QKeySequence>(settings.value(actions[j])));
        }
        m_schemes.insert(schemes[i], shortcuts);

        settings.endGroup();
    }

    // Generate default shortcut scheme on first run and after version updates
    if (!m_schemes.contains(QLatin1String("Default")) || (version != QCoreApplication::applicationVersion()))
        createDefaultScheme();
    if (!m_schemes.contains(m_currentScheme))
        m_currentScheme = QLatin1String("Default");

    m_loaded = true;
}

void ShortcutManager::createDefaultScheme()
{
    QMultiHash<Action, QKeySequence> scheme;

    scheme.insert(NewWindow, QKeySequence::New);
    scheme.insert(NewTab, QKeySequence::AddTab);
    scheme.insert(OpenFile, QKeySequence::Open);
    // Add the location bar shortcuts familiar to users from other browsers
    scheme.insert(OpenLocation, QKeySequence(Qt::ControlModifier + Qt::Key_L));
    scheme.insert(OpenLocation, QKeySequence(Qt::AltModifier + Qt::Key_O));
    scheme.insert(OpenLocation, QKeySequence(Qt::AltModifier + Qt::Key_D));
    // TODO
#if 0
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
#endif

    m_schemes.insert(QLatin1String("Default"), scheme);
}

void ShortcutManager::init()
{
    if (!m_nameToAction.isEmpty())
        return;

    for (int i = 0; i < _NumActions; i++)
        m_nameToAction.insert(shortcutName((Action)i), (Action)i);
}
