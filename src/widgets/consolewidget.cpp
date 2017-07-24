#include "consolewidget.h"
#include "ui_consolewidget.h"

#include "helpers.h"
#include "iaitorcore.h"

#include <QScrollBar>
#include <QMenu>
#include <QCompleter>
#include <QAction>
#include <QShortcut>
#include <QStringListModel>


// TODO: Find a way to get to this without copying it here
// source: libr/core/core.c:585..
// remark: u.* is missing
static const QStringList radareArgs(
{
    "?", "?v", "whereis", "which", "ls", "rm", "mkdir", "pwd", "cat", "less",
    "dH", "ds", "dso", "dsl", "dc", "dd", "dm", "db ", "db-",
    "dp", "dr", "dcu", "dmd", "dmp", "dml",
    "ec", "ecs", "eco",
    "S", "S.", "S*", "S-", "S=", "Sa", "Sa-", "Sd", "Sl", "SSj", "Sr",
    "s", "s+", "s++", "s-", "s--", "s*", "sa", "sb", "sr",
    "!", "!!",
    "#sha1", "#crc32", "#pcprint", "#sha256", "#sha512", "#md4", "#md5",
    "#!python", "#!perl", "#!vala",
    "V", "v",
    "aa", "ab", "af", "ar", "ag", "at", "a?", "ax", "ad",
    "ae", "aec", "aex", "aep", "aea", "aeA", "aes", "aeso", "aesu", "aesue", "aer", "aei", "aeim", "aef",
    "aaa", "aac", "aae", "aai", "aar", "aan", "aas", "aat", "aap", "aav",
    "af", "afa", "afan", "afc", "afC", "afi", "afb", "afbb", "afn", "afr", "afs", "af*", "afv", "afvn",
    "aga", "agc", "agd", "agl", "agfl",
    // see forbbidenArgs
    //"e", "et", "e-", "e*", "e!", "e?", "env ",
    "i", "ii", "iI", "is", "iS", "iz",
    "q", "q!",
    "f", "fl", "fr", "f-", "f*", "fs", "fS", "fr", "fo", "f?",
    "m", "m*", "ml", "m-", "my", "mg", "md", "mp", "m?",
    "o", "o+", "oc", "on", "op", "o-", "x", "wf", "wF", "wta", "wtf", "wp",
    "t", "to", "t-", "tf", "td", "td-", "tb", "tn", "te", "tl", "tk", "ts", "tu",
    "(", "(*", "(-", "()", ".", ".!", ".(", "./",
    "r", "r+", "r-",
    "b", "bf", "b?",
    "/", "//", "/a", "/c", "/h", "/m", "/x", "/v", "/v2", "/v4", "/v8", "/r", "/re",
    "y", "yy", "y?",
    "wx", "ww", "w?", "wxf",
    "p6d", "p6e", "p8", "pb", "pc",
    "pd", "pda", "pdb", "pdc", "pdj", "pdr", "pdf", "pdi", "pdl", "pds", "pdt",
    "pD", "px", "pX", "po", "pf", "pf.", "pf*", "pf*.", "pfd", "pfd.", "pv", "p=", "p-",
    "pfj", "pfj.", "pfv", "pfv.",
    "pm", "pr", "pt", "ptd", "ptn", "pt?", "ps", "pz", "pu", "pU", "p?",
    "z", "z*", "zj", "z-", "z-*",
    "za", "zaf", "zaF",
    "zo", "zoz", "zos",
    "zfd", "zfs", "zfz",
    "z/", "z/*",
    "zc",
    "zs", "zs+", "zs-", "zs-*", "zsr",
    "#!pipe"
});

static const QStringList forbiddenArgs({"e", "et", "e-", "e*", "e!", "e?", "env"});


static const int invalidHistoryPos = -1;



static bool isForbidden(const QString &input)
{
    static const QRegExp delimiters("[;&]");


    const QStringList &commands = input.split(delimiters, QString::SkipEmptyParts);

    for (const QString &command : commands)
    {
        const QString &trimmedCommand = command.trimmed();

        if (forbiddenArgs.contains(trimmedCommand)) return true;

        for (const QString &arg : forbiddenArgs)
        {
            if (trimmedCommand.startsWith(arg + " ")) return true;
        }
    }

    return false;
}



ConsoleWidget::ConsoleWidget(IaitoRCore *core, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConsoleWidget),
    core(core),
    debugOutputEnabled(true),
    maxHistoryEntries(100),
    lastHistoryPosition(invalidHistoryPos)
{
    ui->setupUi(this);

    // Adjust console lineedit
    ui->inputLineEdit->setTextMargins(10, 0, 0, 0);

    /*
    ui->consoleOutputTextEdit->setFont(QFont("Monospace", 8));
    ui->consoleOutputTextEdit->setStyleSheet("background-color:black;color:gray;");
    ui->consoleInputLineEdit->setStyleSheet("background-color:black;color:gray;");
    */

    // Adjust text margins of consoleOutputTextEdit
    QTextDocument *console_docu = ui->outputTextEdit->document();
    console_docu->setDocumentMargin(10);

    // Fix output panel font
    qhelpers::normalizeFont(ui->outputTextEdit);

    QAction *action = new QAction(tr("Clear ouput"), ui->outputTextEdit);
    connect(action, SIGNAL(triggered(bool)), ui->outputTextEdit, SLOT(clear()));
    actions.append(action);

    action = new QAction(tr("Sync with core"), ui->outputTextEdit);
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(syncWithCoreToggled(bool)));
    actions.append(action);

    // Completion
    QCompleter *completer = new QCompleter(radareArgs, this);
    completer->setMaxVisibleItems(20);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);

    ui->inputLineEdit->setCompleter(completer);

    // Set console output context menu
    ui->outputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->outputTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showCustomContextMenu(const QPoint &)));

    // Esc clears inputLineEdit (like OmniBar)
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), ui->inputLineEdit);
    connect(clear_shortcut, SIGNAL(activated()), this, SLOT(clear()));
    clear_shortcut->setContext(Qt::WidgetShortcut);

    // Up and down arrows show history
    QShortcut *historyOnUp = new QShortcut(QKeySequence(Qt::Key_Up), ui->inputLineEdit);
    connect(historyOnUp, SIGNAL(activated()), this, SLOT(historyPrev()));
    historyOnUp->setContext(Qt::WidgetShortcut);

    QShortcut *historyOnDown = new QShortcut(QKeySequence(Qt::Key_Down), ui->inputLineEdit);
    connect(historyOnDown, SIGNAL(activated()), this, SLOT(historyNext()));
    historyOnDown->setContext(Qt::WidgetShortcut);
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
}

void ConsoleWidget::addOutput(const QString &msg)
{
    ui->outputTextEdit->appendPlainText(msg);
    scrollOutputToEnd();
}

void ConsoleWidget::addDebugOutput(const QString &msg)
{
    if (debugOutputEnabled)
    {
        ui->outputTextEdit->appendHtml("<font color=\"red\"> [DEBUG]:\t" + msg + "</font>");
        scrollOutputToEnd();
    }
}

void ConsoleWidget::focusInputLineEdit()
{
    ui->inputLineEdit->setFocus();
}

void ConsoleWidget::on_inputLineEdit_returnPressed()
{
    QString input = ui->inputLineEdit->text();
    if (!input.isEmpty() && core != nullptr)
    {
        if (!isForbidden(input))
        {
            ui->outputTextEdit->appendPlainText(this->core->cmd(input));
            scrollOutputToEnd();

            historyAdd(input);
        }
        else
        {
            addDebugOutput(tr("command forbidden: ") + input);
        }

        ui->inputLineEdit->clear();
    }
}

void ConsoleWidget::on_execButton_clicked()
{
    on_inputLineEdit_returnPressed();
}

void ConsoleWidget::showCustomContextMenu(const QPoint &pt)
{
    QMenu *menu = new QMenu(ui->outputTextEdit);
    menu->addActions(actions);
    menu->exec(ui->outputTextEdit->mapToGlobal(pt));
    menu->deleteLater();
}

void ConsoleWidget::syncWithCoreToggled(bool checked)
{
    if (checked)
    {
        //Enable core syncronization
    }
    else
    {
        // Disable core sync
    }
}

void ConsoleWidget::historyNext()
{
    if (!history.isEmpty())
    {
        if (lastHistoryPosition > invalidHistoryPos)
        {
            if (lastHistoryPosition >= history.size())
            {
                lastHistoryPosition = history.size() - 1 ;
            }

            --lastHistoryPosition;

            if (lastHistoryPosition >= 0)
            {
                ui->inputLineEdit->setText(history.at(lastHistoryPosition));
            }
            else
            {
                ui->inputLineEdit->clear();
            }


        }
    }
}

void ConsoleWidget::historyPrev()
{
    if (!history.isEmpty())
    {
        if (lastHistoryPosition >= history.size() - 1)
        {
            lastHistoryPosition = history.size() - 2;
        }

        ui->inputLineEdit->setText(history.at(++lastHistoryPosition));
    }
}

void ConsoleWidget::clear()
{
    ui->inputLineEdit->clear();

    invalidateHistoryPosition();

    // Close the potential shown completer popup
    ui->inputLineEdit->clearFocus();
    ui->inputLineEdit->setFocus();
}

void ConsoleWidget::scrollOutputToEnd()
{
    const int maxValue = ui->outputTextEdit->verticalScrollBar()->maximum();
    ui->outputTextEdit->verticalScrollBar()->setValue(maxValue);
}

void ConsoleWidget::historyAdd(const QString &input)
{
    if (history.size() + 1 > maxHistoryEntries)
    {
        history.removeLast();
    }

    history.prepend(input);

    invalidateHistoryPosition();
}

void ConsoleWidget::invalidateHistoryPosition()
{
    lastHistoryPosition = invalidHistoryPos;
}
