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

#include "shortcuts.h"

#include <qcoreapplication.h>
#include <qdesktopservices.h>
#include <qstringlist.h>
#include <qsettings.h>

bool Shortcuts::m_loaded = false;
QHash<QString, Shortcuts::Scheme > Shortcuts::m_schemes;
QString Shortcuts::m_currentScheme = QLatin1String("Default");
QHash<QString, Shortcuts::Action> Shortcuts::m_nameToAction;


QList<QKeySequence> Shortcuts::shortcutsFor(Action action)
{
    load();
    return m_schemes.value(m_currentScheme).values(action);
}

QList<QKeySequence> Shortcuts::shortcutsFor(const QString &name)
{
    load();
    return shortcutsFor(shortcutAction(name));
}

QString Shortcuts::shortcutName(Action action)
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

Shortcuts::Action Shortcuts::shortcutAction(const QString &name)
{
    init();
    return m_nameToAction.value(name, NoAction);
}

Shortcuts::Scheme Shortcuts::scheme(const QString &name)
{
    return m_schemes.value(name);
}

// The standard QString operator< is case sensitive
static bool stringLessThan(const QString &a, const QString &b)
{
    return (a.compare(b, Qt::CaseInsensitive) < 0);
}

QStringList Shortcuts::schemes()
{
    QStringList keys = m_schemes.keys();
    qSort(keys.begin(), keys.end(), stringLessThan);
    return keys;
}

Shortcuts::Scheme Shortcuts::currentScheme()
{
    return m_schemes.value(m_currentScheme);
}

QString Shortcuts::currentSchemeName()
{
    return m_currentScheme;
}

void Shortcuts::setCurrentScheme(const QString &name)
{
    if (m_schemes.contains(name))
        m_currentScheme = name;
}

// Everybody loves templates...
void Shortcuts::retranslate()
{
    return;
    // Generate a list of actions that are still translatable, i.e. that
    // are also present in the default scheme (for the same action).
    // We can use absolute sequence list indexes here because they won't
    // change during this function.
    QHash<QString, QMultiHash<Action, QPair<int, int> > > retranslateSequences;
    QStringList schemeNames = m_schemes.keys();
    QLatin1String def = QLatin1String("Default");
    for (int i = 0; i < schemeNames.count(); ++i) {
        if (schemeNames[i] == def)
            continue;
        QMultiHash<Action, QPair<int, int> > temp;
        for (int j = 0; j < _NumActions; ++j) {
            QList<QKeySequence> sequences = m_schemes[schemeNames[i]].values((Action)j);
            QList<QKeySequence> defSequences = m_schemes[def].values((Action)j);
            for (int k = 0; k < sequences.count(); ++k) {
                int l = defSequences.indexOf(sequences[k]);
                if (l>= 0) {
                    temp.insert((Action)j, QPair<int, int>(k, l));
                    break;
                }
            }
        }
        retranslateSequences.insert(schemeNames[i], temp);
    }

    // Regenerate the default schemes
    createDefaultSchemes();

    // Copy retranslated shortcuts from default scheme
    QHashIterator<QString, QMultiHash<Action, QPair<int, int> > > it(retranslateSequences);
    while (it.hasNext()) {
        it.next();
        QMultiHash<Action, QPair<int, int> > sequences = it.value();
        // An iterator would probably be faster, but is a inconvenient because
        // of the QMultiHash.
        QList<Action> actions = sequences.keys();
        for (int j = 0; j < actions.count(); ++j) {
            QList<QKeySequence> list = m_schemes[it.key()].values(actions[j]);
            QList<QKeySequence> defList = m_schemes[def].values(actions[j]);
            QList<QPair<int, int> > pairs = sequences.values(actions[j]);
            for (int k = 0; k < pairs.count(); ++k) {
                list[pairs[k].first] = list[pairs[k].second];
            }
        }
    }
}

void Shortcuts::save()
{
    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QString shortcutsFile = dir + QLatin1String("/shortcuts.conf");
    QSettings settings(shortcutsFile, QSettings::IniFormat);

    settings.clear();
    settings.setValue(QLatin1String("aroraVersion"), QCoreApplication::applicationVersion());

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
            settings.setValue(QString(QLatin1String("%1_%2")).arg(k).arg(shortcutName(j.key())), j.value());
            ++k;
        }

        settings.endGroup();
    }

    QSettings appSettings;
    appSettings.beginGroup(QLatin1String("shortcuts"));
    appSettings.setValue(QLatin1String("currentScheme"), m_currentScheme);
    appSettings.endGroup();
}

void Shortcuts::load()
{
    if (m_loaded)
        return;

    init();

    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QString shortcutsFile = dir + QLatin1String("/shortcuts.conf");
    QSettings settings(shortcutsFile, QSettings::IniFormat);

    QString version = settings.value(QLatin1String("aroraVersion")).toString();

    QStringList schemes = settings.childGroups();
    m_schemes.clear();
    for (int i = 0; i < schemes.count(); ++i) {
        settings.beginGroup(schemes[i]);

        QMultiHash<Action, QKeySequence> shortcuts;
        QStringList actions = settings.childKeys();
        for (int j = 0; j < actions.count(); ++j) {
            // See comment in save() for an explaination
            QStringList tmp = actions[j].split(QLatin1Char('_'));
            if (tmp.count() < 2) {
                continue;
            }
            shortcuts.insert(shortcutAction(tmp[1]), qvariant_cast<QKeySequence>(settings.value(actions[j])));
        }
        m_schemes.insert(schemes[i], shortcuts);

        settings.endGroup();
    }

    QSettings appSettings;
    appSettings.beginGroup(QLatin1String("shortcuts"));
    m_currentScheme = appSettings.value(QLatin1String("currentScheme"), QLatin1String("Default")).toString();
    appSettings.endGroup();

    // Generate default shortcut scheme on first run and after version updates
    if (!m_schemes.contains(QLatin1String("Default")) || (version != QCoreApplication::applicationVersion()))
        createDefaultSchemes();
    if (!m_schemes.contains(m_currentScheme))
        m_currentScheme = QLatin1String("Default");

    m_loaded = true;
}

void Shortcuts::createDefaultSchemes()
{
    QMultiHash<Action, QKeySequence> scheme;

    scheme.insert(NewWindow, QKeySequence::New);
    scheme.insert(NewTab, QKeySequence::AddTab);
    scheme.insert(OpenFile, QKeySequence::Open);
    // Add the location bar shortcuts familiar to users from other browsers
    scheme.insert(OpenLocation, QKeySequence(Qt::ControlModifier + Qt::Key_L));
    scheme.insert(OpenLocation, QKeySequence(Qt::AltModifier + Qt::Key_O));
    scheme.insert(OpenLocation, QKeySequence(Qt::AltModifier + Qt::Key_D));
    scheme.insert(CloseTab, QKeySequence::Close);
    scheme.insert(SaveAs, QKeySequence::Save);
    scheme.insert(Print, QKeySequence::Print);
    // PrivateBrowsing is empty
    scheme.insert(CloseWindow, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));

    scheme.insert(Find, QKeySequence::Find);
    scheme.insert(FindNext, QKeySequence::FindNext);
    scheme.insert(FindPrevious, QKeySequence::FindPrevious);
    scheme.insert(Preferences, tr("Ctrl+,"));

    scheme.insert(NextTab, QKeySequence(Qt::CTRL | Qt::Key_BraceRight));
    scheme.insert(NextTab, QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    scheme.insert(NextTab, tr("Ctrl+]"));
    scheme.insert(NextTab, QKeySequence(Qt::CTRL | Qt::Key_Less));
    scheme.insert(NextTab, QKeySequence(Qt::CTRL | Qt::Key_Tab));

    scheme.insert(PreviousTab, QKeySequence(Qt::CTRL | Qt::Key_BraceLeft));
    scheme.insert(PreviousTab, QKeySequence(Qt::CTRL | Qt::Key_PageUp));
    scheme.insert(PreviousTab, tr("Ctrl+["));
    scheme.insert(PreviousTab, QKeySequence(Qt::CTRL | Qt::Key_Greater));
    scheme.insert(PreviousTab, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    scheme.insert(PageSource, tr("Ctrl+Alt+U"));

    scheme.insert(WebSearch, QKeySequence(tr("Ctrl+K", "Web Search")));
    scheme.insert(ClearPrivateData, QKeySequence(tr("Ctrl+Shift+Delete", "Clear Private Data")));
#if 0
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

void Shortcuts::init()
{
    if (!m_nameToAction.isEmpty())
        return;

    for (int i = 0; i < _NumActions; ++i)
        m_nameToAction.insert(shortcutName((Action)i), (Action)i);
}

// Returns the name of the new scheme (might be different if it is a default
// scheme which can't be overridden)
QString Shortcuts::saveScheme(const QString &name, const Scheme &scheme)
{
    if (name != QLatin1String("Default")) {
        m_schemes.insert(name, scheme);
        save();
        return name;
    }

    QString newName;
    int i = 0;
    do {
        newName = QString(QLatin1String("%1_%2")).arg(name).arg(i++);
    } while (!m_schemes.contains(newName));

    m_schemes.insert(newName, scheme);
    save();
    return newName;
}

