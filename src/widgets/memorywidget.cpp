#include "memorywidget.h"
#include "ui_memorywidget.h"

#include "mainwindow.h"
#include "helpers.h"
#include "dialogs/xrefsdialog.h"
#include "dialogs/renamedialog.h"
#include "dialogs/commentsdialog.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QWebEnginePage>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QSettings>

#include <cassert>

MemoryWidget::MemoryWidget(MainWindow *main) :
    DockWidget(main),
    ui(new Ui::MemoryWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    this->disasTextEdit = ui->disasTextEdit_2;
    this->hexOffsetText = ui->hexOffsetText_2;
    this->hexHexText = ui->hexHexText_2;
    this->hexDisasTextEdit = ui->hexDisasTextEdit_2;
    this->hexASCIIText = ui->hexASCIIText_2;
    this->xrefToTreeWidget_2 = ui->xrefToTreeWidget_2;
    this->xreFromTreeWidget_2 = ui->xreFromTreeWidget_2;
    this->memTabWidget = ui->memTabWidget;

    this->last_fcn = "entry0";
    this->last_disasm_fcn = 0; //"";
    this->last_graph_fcn = 0; //"";
    this->last_hexdump_fcn = 0; //"";

    // Increase asm text edit margin
    QTextDocument *asm_docu = this->disasTextEdit->document();
    asm_docu->setDocumentMargin(10);

    // Setup disasm highlight
    connect(ui->disasTextEdit_2, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    highlightCurrentLine();
    //this->on_actionSettings_menu_1_triggered();

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    // Highlight current line on previews and decompiler
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightPreviewCurrentLine()));
    connect(ui->decoTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightDecoCurrentLine()));

    // Hide memview notebooks tabs
    QTabBar *bar = ui->memTabWidget->tabBar();
    bar->setVisible(false);
    QTabBar *sidebar = ui->memSideTabWidget_2->tabBar();
    sidebar->setVisible(false);
    QTabBar *preTab = ui->memPreviewTab->tabBar();
    preTab->setVisible(false);

    // Hide fcn graph notebooks tabs
    QTabBar *graph_bar = ui->fcnGraphTabWidget->tabBar();
    graph_bar->setVisible(false);

    // Hide graph webview scrollbars
    ui->graphWebView->page()->runJavaScript("document.body.style.overflow='hidden';");

    // Allows the local resources (qrc://) to access http content
    if (!ui->graphWebView->settings()->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls))
    {
        ui->graphWebView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    }

    // Debug console
    // For QWebEngine debugging see: https://doc.qt.io/qt-5/qtwebengine-debugging.html
    //QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    // Add margin to function name line edit
    ui->fcnNameEdit->setTextMargins(5, 0, 0, 0);

    // Normalize fonts for other OS
    qhelpers::normalizeFont(this->disasTextEdit);
    qhelpers::normalizeEditFont(this->hexOffsetText);
    qhelpers::normalizeEditFont(this->hexHexText);
    qhelpers::normalizeEditFont(this->hexASCIIText);

    // Popup menu on Settings toolbutton
    QMenu *memMenu = new QMenu();
    ui->memSettingsButton_2->addAction(ui->actionSettings_menu_1);
    memMenu->addAction(ui->actionSettings_menu_1);
    QMenu *hideSide = memMenu->addMenu("Show/Hide");
    hideSide->addAction(ui->actionDisas_ShowHideBytes);
    hideSide->addAction(ui->actionSeparate_bytes);
    hideSide->addAction(ui->actionRight_align_bytes);
    hideSide->addSeparator();
    hideSide->addAction(ui->actionDisasSwitch_case);
    hideSide->addAction(ui->actionSyntax_AT_T_Intel);
    hideSide->addAction(ui->actionSeparate_disasm_calls);
    hideSide->addAction(ui->actionShow_stack_pointer);
    ui->memSettingsButton_2->setMenu(memMenu);

    // Disable bytes options by default as bytes are not shown
    ui->actionSeparate_bytes->setDisabled(true);
    ui->actionRight_align_bytes->setDisabled(true);

    // Event filter to intercept double clicks in the textbox
    ui->disasTextEdit_2->viewport()->installEventFilter(this);

    // Set Splitter stretch factor
    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);

    // Set Disas context menu
    ui->disasTextEdit_2->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->disasTextEdit_2, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));

    // Set hexdump context menu
    ui->hexHexText_2->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexHexText_2, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexdumpContextMenu(const QPoint &)));
    ui->hexASCIIText_2->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->hexASCIIText_2, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showHexASCIIContextMenu(const QPoint &)));

    // Syncronize hexdump scrolling
    connect(ui->hexOffsetText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexOffsetText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText_2->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexHexText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexHexText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexASCIIText_2->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->hexASCIIText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexOffsetText_2->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->hexASCIIText_2->verticalScrollBar(), SIGNAL(valueChanged(int)),
            ui->hexHexText_2->verticalScrollBar(), SLOT(setValue(int)));

    // X to show hexdump
    QShortcut *hexdump_shortcut = new QShortcut(QKeySequence(Qt::Key_X), this->main);
    connect(hexdump_shortcut, SIGNAL(activated()), this, SLOT(showHexdump()));
    //hexdump_shortcut->setContext(Qt::WidgetShortcut);

    // Space to switch between disassembly and graph
    QShortcut *graph_shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this->main);
    connect(graph_shortcut, SIGNAL(activated()), this, SLOT(cycleViews()));
    //graph_shortcut->setContext(Qt::WidgetShortcut);

    // Semicolon to add comment
    QShortcut *comment_shortcut = new QShortcut(QKeySequence(Qt::Key_Semicolon), ui->disasTextEdit_2);
    connect(comment_shortcut, SIGNAL(activated()), this, SLOT(on_actionDisasAdd_comment_triggered()));
    comment_shortcut->setContext(Qt::WidgetShortcut);

    // N to rename function
    QShortcut *rename_shortcut = new QShortcut(QKeySequence(Qt::Key_N), ui->disasTextEdit_2);
    connect(rename_shortcut, SIGNAL(activated()), this, SLOT(on_actionFunctionsRename_triggered()));
    rename_shortcut->setContext(Qt::WidgetShortcut);

    // R to show XRefs
    QShortcut *xrefs_shortcut = new QShortcut(QKeySequence(Qt::Key_R), ui->disasTextEdit_2);
    connect(xrefs_shortcut, SIGNAL(activated()), this, SLOT(on_actionXRefs_triggered()));
    xrefs_shortcut->setContext(Qt::WidgetShortcut);

    // Esc to seek back
    QShortcut *back_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), ui->disasTextEdit_2);
    connect(back_shortcut, SIGNAL(activated()), this, SLOT(seek_back()));
    back_shortcut->setContext(Qt::WidgetShortcut);

    // CTRL + R to refresh the disasm
    QShortcut *refresh_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), ui->disasTextEdit_2);
    connect(refresh_shortcut, SIGNAL(activated()), this, SLOT(refreshDisasm()));
    refresh_shortcut->setContext(Qt::WidgetShortcut);

    // Control Disasm and Hex scroll to add more contents
    connect(this->disasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    connect(ui->graphWebView->page(), SIGNAL(loadFinished(bool)), this, SLOT(frameLoadFinished(bool)));

    connect(main, SIGNAL(cursorAddressChanged(RVA)), this, SLOT(on_cursorAddressChanged(RVA)));

    fillPlugins();
}



void MemoryWidget::on_cursorAddressChanged(RVA addr)
{
    setFcnName(addr);
}

/*
 * Text highlight functions
 */

void MemoryWidget::highlightDisasms()
{
    // Syntax Highliting
    highlighter = new Highlighter(this->main, ui->disasTextEdit_2->document());
    highlighter_5 = new Highlighter(this->main, ui->hexDisasTextEdit_2->document());
    ascii_highlighter = new AsciiHighlighter(ui->hexASCIIText_2->document());
    hex_highlighter = new HexHighlighter(ui->hexHexText_2->document());
    preview_highlighter = new Highlighter(this->main, ui->previewTextEdit->document());
    deco_highlighter = new Highlighter(this->main, ui->decoTextEdit->document());

}

void MemoryWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Highlight the current line in yellow
    if (ui->disasTextEdit_2->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->disasTextEdit_2->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // Highlight the current word
    QTextCursor cursor = ui->disasTextEdit_2->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);
    currentWord.cursor.clearSelection();

    // Highlight all the words in the document same as the actual one
    QString searchString = cursor.selectedText();
    QTextDocument *document = ui->disasTextEdit_2->document();

    //QTextCursor highlightCursor(document);
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = cursor;
    highlightSelection.format.setBackground(blueColor);
    QTextCursor cursor2(document);

    cursor2.beginEditBlock();

    highlightSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd())
    {
        highlightSelection.cursor = document->find(searchString, highlightSelection.cursor, QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull())
        {
            highlightSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(highlightSelection);
        }
    }
    cursor2.endEditBlock();

    ui->disasTextEdit_2->setExtraSelections(extraSelections);
}

void MemoryWidget::highlightHexCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!ui->hexHexText_2->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->hexHexText_2->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    QTextCursor cursor = ui->hexHexText_2->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);

    ui->hexHexText_2->setExtraSelections(extraSelections);

    highlightHexWords(cursor.selectedText());
}

void MemoryWidget::highlightHexWords(const QString &str)
{
    QString searchString = str;
    QTextDocument *document = ui->hexHexText_2->document();

    document->undo();

    QTextCursor highlightCursor(document);
    QTextCursor cursor(document);

    cursor.beginEditBlock();

    QColor blueColor = QColor(Qt::blue).lighter(160);

    QTextCharFormat plainFormat(highlightCursor.charFormat());
    QTextCharFormat colorFormat = plainFormat;
    colorFormat.setBackground(blueColor);

    while (!highlightCursor.isNull() && !highlightCursor.atEnd())
    {
        highlightCursor = document->find(searchString, highlightCursor, QTextDocument::FindWholeWords);

        if (!highlightCursor.isNull())
        {
            highlightCursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            highlightCursor.mergeCharFormat(colorFormat);
        }
    }
    cursor.endEditBlock();
}

void MemoryWidget::highlightPreviewCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->toPlainText() != "")
    {
        if (ui->previewTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->previewTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->previewTextEdit->setExtraSelections(extraSelections);
}

void MemoryWidget::highlightDecoCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->decoTextEdit->toPlainText() != "")
    {
        if (ui->decoTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->decoTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->decoTextEdit->setExtraSelections(extraSelections);
}


MemoryWidget::~MemoryWidget()
{
    delete ui;
}

void MemoryWidget::setup()
{
    setScrollMode();

    const QString off = main->core->cmd("afo entry0").trimmed();

    refreshDisasm(off);
    refreshHexdump(off);
    create_graph(off);
    get_refs_data(off);
    //setFcnName(off);
}

void MemoryWidget::refresh()
{
    setScrollMode();

    // TODO: honor the offset
    updateViews();
}

/*
 * Content management functions
 */

void MemoryWidget::fillPlugins()
{
    // Fill the plugins combo for the hexdump sidebar
    ui->hexArchComboBox_2->insertItems(0, main->core->getAsmPluginNames());
}

void MemoryWidget::addTextDisasm(QString txt)
{
    //QTextDocument *document = ui->disasTextEdit_2->document();
    //document->undo();
    ui->disasTextEdit_2->appendPlainText(txt);
}

void MemoryWidget::replaceTextDisasm(QString txt)
{
    //QTextDocument *document = ui->disasTextEdit_2->document();
    ui->disasTextEdit_2->clear();
    //document->undo();
    ui->disasTextEdit_2->setPlainText(txt);
}

void MemoryWidget::disasmScrolled()
{
    /*
     * Add more disasm as the user scrolls
     * Not working properly when scrolling upwards
     * r2 doesn't handle properly 'pd-' for archs with variable instruction size
     */

    // Disconnect scroll signals to add more content
    disconnect(this->disasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));

    QScrollBar *sb = this->disasTextEdit->verticalScrollBar();

    if (sb->value() > sb->maximum() - 10)
    {
        //this->main->add_debug_output("End is coming");

        QTextCursor tc = this->disasTextEdit->textCursor();
        tc.movePosition(QTextCursor::End);
        tc.select(QTextCursor::LineUnderCursor);
        QString lastline = tc.selectedText();
        QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
        if (ele.contains("0x"))
        {
            this->main->core->seek(ele);
            QString raw = this->main->core->cmd("pd 200");
            QString txt = raw.section("\n", 1, -1);
            //this->disasTextEdit->appendPlainText(" ;\n ; New content here\n ;\n " + txt.trimmed());
            this->disasTextEdit->appendPlainText(txt.trimmed());
        }
        else
        {
            tc.movePosition(QTextCursor::End);
            tc.select(QTextCursor::LineUnderCursor);
            QString lastline = tc.selectedText();
            this->main->addDebugOutput("Last line: " + lastline);
        }
        // Code below will be used to append more disasm upwards, one day
    } /* else if (sb->value() < sb->minimum() + 10) {
        //this->main->add_debug_output("Begining is coming");

        QTextCursor tc = this->disasTextEdit->textCursor();
        tc.movePosition( QTextCursor::Start );
        tc.select( QTextCursor::LineUnderCursor );
        QString firstline = tc.selectedText();
        //this->main->add_debug_output("First Line: " + firstline);
        QString ele = firstline.split(" ", QString::SkipEmptyParts)[0];
        //this->main->add_debug_output("First Offset: " + ele);
        if (ele.contains("0x")) {
            int b = this->disasTextEdit->verticalScrollBar()->maximum();
            this->main->core->cmd("ss " + ele);
            this->main->core->cmd("so -50");
            QString raw = this->main->core->cmd("pd 50");
            //this->main->add_debug_output(raw);
            //QString txt = raw.section("\n", 1, -1);
            //this->main->add_debug_output(txt);
            tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            //tc.insertText(raw.trimmed() + "\n ;\n ; New content prepended here\n ;\n");
            int c = this->disasTextEdit->verticalScrollBar()->maximum();
            int z = c -b;
            int a = this->disasTextEdit->verticalScrollBar()->sliderPosition();
            this->disasTextEdit->verticalScrollBar()->setValue(a + z);
        } else {
            tc.movePosition( QTextCursor::Start );
            tc.select( QTextCursor::LineUnderCursor );
            QString lastline = tc.selectedText();
            this->main->add_debug_output("Last line: " + lastline);
        }
    } */

    // Reconnect scroll signals
    connect(this->disasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
}

void MemoryWidget::refreshDisasm(const QString &offset)
{
    RCoreLocked lcore = this->main->core->core();
    // we must store those ranges somewhere, to handle scroll
    //ut64 addr = lcore->offset;
    //int length = lcore->num->value;

    //printf("refreshDisasm %s\n", offset.toLocal8Bit().constData());

    // Prevent further scroll
    disconnect(this->disasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    disconnect(this->disasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(on_disasTextEdit_2_cursorPositionChanged()));

    // Get disas at offset
    if (!offset.isEmpty())
    {
        this->main->core->cmd("s " + offset);
    }
    else
    {
        // Get current offset
        QTextCursor tc = this->disasTextEdit->textCursor();
        tc.select(QTextCursor::LineUnderCursor);
        QString lastline = tc.selectedText();
        QStringList elements = lastline.split(" ", QString::SkipEmptyParts);
        if (elements.length() > 0)
        {
            QString ele = elements[0];
            if (ele.contains("0x"))
            {
                QString fcn = this->main->core->cmdFunctionAt(ele);
                if (fcn != "")
                {
                    this->main->core->cmd("s " + fcn);
                }
                else
                {
                    this->main->core->cmd("s " + ele);
                }
            }
        }
    }

    QString txt2 = this->main->core->cmd("pd 100");
    this->disasTextEdit->setPlainText(txt2.trimmed());

    // TODO: Fixx this ugly code
    //QString temp_seek = this->main->core->cmd("s").split("0x")[1].trimmed();
    QString s = this->normalize_addr(this->main->core->cmd("s"));
    //this->main->add_debug_output("Offset to search: " + s);
    this->disasTextEdit->ensureCursorVisible();
    /*this->disasTextEdit->moveCursor(QTextCursor::End);

    while (this->disasTextEdit->find(QRegExp("^" + s), QTextDocument::FindBackward))
    {
        this->disasTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    }*/

    connect(this->disasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    connect(this->disasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(on_disasTextEdit_2_cursorPositionChanged()));
    this->on_disasTextEdit_2_cursorPositionChanged();
}

void MemoryWidget::refreshHexdump(const QString &where)
{
    RCoreLocked lcore = this->main->core->core();
    // Prevent further scroll
    disconnect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));

    // Clear previous content to add new
    this->hexOffsetText->clear();
    this->hexHexText->clear();
    this->hexASCIIText->clear();

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (hexdumpBottomOffset < bsize)
    {
        hexdumpBottomOffset = 0;
        hexdumpLength = bsize;//-hexdumpBottomOffset;
    }
    else
    {
        hexdumpLength = bsize;
    }

    //int size;
    //size = main->core->get_size();

    QString s = "";

    if (!where.isEmpty())
    {
        this->main->core->cmd("ss " + where);
    }

    // Add first the hexdump at block size --
    this->main->core->cmd("ss-" + this->main->core->itoa(hexdumpLength));
    //s = this->normalize_addr(this->main->core->cmd("s"));
    QList<QString> ret = this->get_hexdump("");

    hexdumpBottomOffset = lcore->offset;
    this->hexOffsetText->setPlainText(ret[0]);
    this->hexHexText->setPlainText(ret[1]);
    this->hexASCIIText->setPlainText(ret[2]);
    this->resizeHexdump();

    // Add then the hexdump at block size ++
    this->main->core->cmd("ss+" + this->main->core->itoa(hexdumpLength));
    // Get address to move cursor to later
    //QString s = "0x0" + this->main->core->cmd("s").split("0x")[1].trimmed();
    s = this->normalize_addr(this->main->core->cmd("s"));
    ret = this->get_hexdump("");

    hexdumpBottomOffset = lcore->offset;
    this->hexOffsetText->append(ret[0]);
    this->hexHexText->append(ret[1]);
    this->hexASCIIText->append(ret[2]);
    this->resizeHexdump();

    // Move cursor to desired address
    QTextCursor cur = this->hexOffsetText->textCursor();
    this->hexOffsetText->ensureCursorVisible();
    this->hexHexText->ensureCursorVisible();
    this->hexASCIIText->ensureCursorVisible();
    this->hexOffsetText->moveCursor(QTextCursor::End);
    this->hexOffsetText->find(s, QTextDocument::FindBackward);
    this->hexOffsetText->moveCursor(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);

    connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
}

QList<QString> MemoryWidget::get_hexdump(const QString &offset)
{
    RCoreLocked lcore = this->main->core->core();
    QList<QString> ret;
    QString hexdump;

    int hexdumpLength;
    int cols = lcore->print->cols;
    ut64 bsize = 128 * cols;
    if (hexdumpBottomOffset < bsize)
    {
        hexdumpBottomOffset = 0;
        hexdumpLength = bsize;
        //-hexdumpBottomOffset;
    }
    else
    {
        hexdumpLength = bsize;
    }

    //this->main->add_debug_output("BSize: " + this->main->core->itoa(hexdumpLength, 10));

    if (offset.isEmpty())
    {
        hexdump = this->main->core->cmd("px " + this->main->core->itoa(hexdumpLength, 10));
    }
    else
    {
        hexdump = this->main->core->cmd("px " + this->main->core->itoa(hexdumpLength, 10) + " @ " + offset);
    }
    //QString hexdump = this->main->core->cmd ("px 0x" + this->main->core->itoa(size) + " @ 0x0");
    // TODO: use pxl to simplify
    QString offsets;
    QString hex;
    QString ascii;
    int ln = 0;

    for (const QString line : hexdump.split("\n"))
    {
        if (ln++ == 0)
        {
            continue;
        }
        int wc = 0;
        for (const QString a : line.split("  "))
        {
            switch (wc++)
            {
            case 0:
                offsets += a + "\n";
                break;
            case 1:
            {
                hex += a.trimmed() + "\n";
            }
            break;
            case 2:
                ascii += a + "\n";
                break;
            }
        }
    }
    ret << offsets.trimmed();
    ret << hex.trimmed();
    ret << ascii.trimmed();

    return ret;
}


void MemoryWidget::seek_to(const QString &offset)
{
    this->disasTextEdit->moveCursor(QTextCursor::End);
    this->disasTextEdit->find(offset, QTextDocument::FindBackward);
    this->disasTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    //this->main->add_debug_output("OFFSET: " + offset);
}

void MemoryWidget::resizeHexdump()
{
    //qDebug() << "size: " << ui->hexHexText->document()->size().width();
    this->hexOffsetText->setMinimumWidth(this->hexOffsetText->document()->size().width());
    this->hexHexText->setMinimumWidth(this->hexHexText->document()->size().width());
    this->hexASCIIText->setMinimumWidth(this->hexASCIIText->document()->size().width());
}

void MemoryWidget::hexScrolled()
{
    RCoreLocked lcore = this->main->core->core();
    QScrollBar *sb = this->hexASCIIText->verticalScrollBar();

    if (sb->value() > sb->maximum() - 10)
    {
        this->main->addDebugOutput("End is coming");

        QTextCursor tc = this->hexOffsetText->textCursor();
        tc.movePosition(QTextCursor::End);
        tc.select(QTextCursor::LineUnderCursor);
        QString lastline = tc.selectedText();
        //this->main->add_debug_output("Last Offset/VA: " + lastline);
        //refreshHexdump(2);

        QList<QString> ret = this->get_hexdump(lastline);

        this->hexOffsetText->append(ret[0]);
        this->hexHexText->append(ret[1]);
        this->hexASCIIText->append(ret[2]);
        this->resizeHexdump();

        // Append more hex text here
        //   ui->disasTextEdit->moveCursor(QTextCursor::Start);
        // ui->disasTextEdit->insertPlainText(core->cmd("pd@$$-100"));
        //... same for the other text (offset and hex text edits)
    }
    else if (sb->value() < sb->minimum() + 10)
    {
        //this->main->add_debug_output("Begining is coming");

        QTextCursor tc = this->hexOffsetText->textCursor();
        tc.movePosition(QTextCursor::Start);
        tc.select(QTextCursor::LineUnderCursor);
        QString firstline = tc.selectedText();
        //disathis->main->add_debug_output("First Offset/VA: " + firstline);
        //refreshHexdump(1);

        //int cols = lcore->print->cols;
        // px bsize @ addr
        //int bsize = 128 * cols;
        int bsize = 800;
        QString s = QString::number(bsize);
        // s = 2048.. sigh...
        QString kk = this->main->core->cmd("? " + firstline + " - " + s);
        QString k = kk.split(" ")[1];

        QList<QString> ret = this->get_hexdump(k);

        // Prevent further scroll
        disconnect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
        // Get actual maximum scrolling value
        int b = this->hexASCIIText->verticalScrollBar()->maximum();

        // Add new offset content
        QTextDocument *offset_document = this->hexOffsetText->document();
        QTextCursor offset_cursor(offset_document);
        offset_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        offset_cursor.insertText(ret[0] + "\n");

        // Add new hex content
        QTextDocument *hex_document = this->hexHexText->document();
        QTextCursor hex_cursor(hex_document);
        hex_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        hex_cursor.insertText(ret[1] + "\n");

        // Add new ASCII content
        QTextDocument *ascii_document = this->hexASCIIText->document();
        QTextCursor ascii_cursor(ascii_document);
        ascii_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        ascii_cursor.insertText(ret[2] + "\n");

        // Get new maximum scroll value
        int c = this->hexASCIIText->verticalScrollBar()->maximum();
        // Get size of new added content
        int z = c - b;
        // Get new slider position
        int a = this->hexASCIIText->verticalScrollBar()->sliderPosition();
        // move to previous position
        this->hexASCIIText->verticalScrollBar()->setValue(a + z);

        this->resizeHexdump();
        connect(this->hexASCIIText->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(hexScrolled()));
    }
}

void MemoryWidget::on_hexHexText_2_selectionChanged()
{
    // Get selected partsing type
    QString parsing = ui->codeCombo_2->currentText();
    // Get selected text
    QTextCursor cursor(this->hexHexText->textCursor());
    QString sel_text = cursor.selectedText();

    sel_text = sel_text.simplified().remove(" ");
    //eprintf ("-- (((%s))) --\n", sel_text.toUtf8().constData());

    if (sel_text == "")
    {
        this->hexDisasTextEdit->setPlainText("");
        ui->bytesEntropy->setText("");
        ui->bytesMD5->setText("");
        ui->bytesSHA1->setText("");
    }
    else
    {
        if (parsing == "Dissasembly")
        {
            // Get selected combos
            QString arch = ui->hexArchComboBox_2->currentText();
            QString bits = ui->hexBitsComboBox_2->currentText();

            QString oarch = this->main->core->config("asm.arch");
            QString obits = this->main->core->config("asm.bits");

            this->main->core->config("asm.arch", arch);
            this->main->core->config("asm.bits", bits);
            QString str = this->main->core->cmd("pad " + sel_text);
            this->hexDisasTextEdit->setPlainText(str);
            this->main->core->config("asm.arch", oarch);
            this->main->core->config("asm.bits", obits);
            //qDebug() << "Selected Arch: " << arch;
            //qDebug() << "Selected Bits: " << bits;
            //qDebug() << "Selected Text: " << sel_text;
        }
        // TODO: update on selection changes.. use cmd("pc "+len+"@"+off)
        else if (parsing == "C byte array")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pc@x:" + sel_text));
        }
        else    if (parsing == "C dword array")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcw@x:" + sel_text));
        }
        else    if (parsing == "C qword array")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcq@x:" + sel_text));
        }
        else    if (parsing == "Assembler")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pca@x:" + sel_text));
        }
        else    if (parsing == "String")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcs@x:" + sel_text));
        }
        else    if (parsing == "JSON")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcj@x:" + sel_text));
        }
        else    if (parsing == "Javascript")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcJ@x:" + sel_text));
        }
        else    if (parsing == "Python")
        {
            this->hexDisasTextEdit->setPlainText(this->main->core->cmd("pcp@x:" + sel_text));
        }

        // Fill the information tab hashes and entropy
        ui->bytesMD5->setText(this->main->core->cmd("ph md5@x:" + sel_text).trimmed());
        ui->bytesSHA1->setText(this->main->core->cmd("ph sha1@x:" + sel_text).trimmed());
        ui->bytesEntropy->setText(this->main->core->cmd("ph entropy@x:" + sel_text).trimmed());
        ui->bytesMD5->setCursorPosition(0);
        ui->bytesSHA1->setCursorPosition(0);
    }
}

void MemoryWidget::on_hexArchComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_2_selectionChanged();
}

void MemoryWidget::on_hexBitsComboBox_2_currentTextChanged(const QString &/*arg1*/)
{
    on_hexHexText_2_selectionChanged();
}

/*
 * Context menu functions
 */

void MemoryWidget::showHexdumpContextMenu(const QPoint &pt)
{
    // Set Hexdump popup menu
    QMenu *menu = ui->hexHexText_2->createStandardContextMenu();
    menu->clear();
    menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();
    QMenu *colSubmenu = menu->addMenu("Columns");
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);
    menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);

    ui->hexHexText_2->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexHexText_2->mapToGlobal(pt));
    delete menu;
}

void MemoryWidget::showHexASCIIContextMenu(const QPoint &pt)
{
    // Set Hex ASCII popup menu
    QMenu *menu = ui->hexASCIIText_2->createStandardContextMenu();
    menu->clear();
    menu->addAction(ui->actionHexCopy_Hexpair);
    menu->addAction(ui->actionHexCopy_ASCII);
    menu->addAction(ui->actionHexCopy_Text);
    menu->addSeparator();
    QMenu *colSubmenu = menu->addMenu("Columns");
    colSubmenu->addAction(ui->action4columns);
    colSubmenu->addAction(ui->action8columns);
    colSubmenu->addAction(ui->action16columns);
    colSubmenu->addAction(ui->action32columns);
    menu->addSeparator();
    menu->addAction(ui->actionHexEdit);
    menu->addAction(ui->actionHexPaste);
    menu->addSeparator();
    menu->addAction(ui->actionHexInsert_Hex);
    menu->addAction(ui->actionHexInsert_String);

    ui->hexASCIIText_2->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->hexASCIIText_2->mapToGlobal(pt));
    delete menu;
}

void MemoryWidget::showDisasContextMenu(const QPoint &pt)
{
    // Set Disas popup menu
    QMenu *menu = ui->disasTextEdit_2->createStandardContextMenu();
    QTextCursor cur = ui->disasTextEdit_2->textCursor();

    // Move cursor to mouse position to get proper function data
    cur.setPosition(ui->disasTextEdit_2->cursorForPosition(pt).position(), QTextCursor::MoveAnchor);
    ui->disasTextEdit_2->setTextCursor(cur);

    if (cur.hasSelection())
    {
        menu->addSeparator();
        menu->addAction(ui->actionSend_to_Notepad);
        ui->disasTextEdit_2->setContextMenuPolicy(Qt::DefaultContextMenu);
    }
    else
    {
        // Add menu actions
        menu->clear();
        menu->addAction(ui->actionDisasAdd_comment);
        menu->addAction(ui->actionFunctionsRename);
        menu->addAction(ui->actionFunctionsUndefine);
        menu->addSeparator();
        menu->addAction(ui->actionXRefs);
        menu->addSeparator();
        menu->addAction(ui->actionDisas_ShowHideBytes);
        menu->addAction(ui->actionSeparate_bytes);
        menu->addAction(ui->actionRight_align_bytes);
        menu->addSeparator();
        menu->addAction(ui->actionDisasSwitch_case);
        menu->addAction(ui->actionSyntax_AT_T_Intel);
        menu->addAction(ui->actionSeparate_disasm_calls);
        menu->addAction(ui->actionShow_stack_pointer);
        menu->addSeparator();
        menu->addAction(ui->actionDisasCopy_All);
        menu->addAction(ui->actionDisasCopy_Bytes);
        menu->addAction(ui->actionDisasCopy_Disasm);

        ui->disasTextEdit_2->setContextMenuPolicy(Qt::CustomContextMenu);
    }
    menu->exec(ui->disasTextEdit_2->mapToGlobal(pt));
    delete menu;
    ui->disasTextEdit_2->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MemoryWidget::on_showInfoButton_2_clicked()
{
    if (ui->showInfoButton_2->isChecked())
    {
        ui->fcnGraphTabWidget->hide();
        ui->showInfoButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->fcnGraphTabWidget->show();
        ui->showInfoButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_offsetToolButton_clicked()
{
    if (ui->offsetToolButton->isChecked())
    {
        ui->offsetTreeWidget->hide();
        ui->offsetToolButton->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->offsetTreeWidget->show();
        ui->offsetToolButton->setArrowType(Qt::DownArrow);
    }
}

/*
 * Show widgets
 */

void MemoryWidget::showHexdump()
{
    ui->hexButton_2->setChecked(true);
    ui->memTabWidget->setCurrentIndex(1);
    ui->memSideTabWidget_2->setCurrentIndex(1);
}

void MemoryWidget::cycleViews()
{
    if (ui->memTabWidget->currentIndex() == 0)
    {
        // Show graph
        ui->graphButton_2->setChecked(true);
        ui->memTabWidget->setCurrentIndex(2);
        ui->memSideTabWidget_2->setCurrentIndex(0);
    }
    else
    {
        // Show disasm
        ui->disButton_2->setChecked(true);
        ui->memTabWidget->setCurrentIndex(0);
        ui->memSideTabWidget_2->setCurrentIndex(0);
    }
}

/*
 * Actions callback functions
 */

void MemoryWidget::on_actionSettings_menu_1_triggered()
{
    bool ok = true;

    // QFont font = QFont("Monospace", 8);

    QFont font = QFontDialog::getFont(&ok, ui->disasTextEdit_2->font(), this);

    if (ok)
    {
        setFonts(font);

        emit fontChanged(font);
    }
}
void MemoryWidget::setFonts(QFont font)
{
    ui->disasTextEdit_2->setFont(font);
    // the user clicked OK and font is set to the font the user selected
    ui->disasTextEdit_2->setFont(font);
    ui->hexOffsetText_2->setFont(font);
    ui->hexHexText_2->setFont(font);
    ui->hexASCIIText_2->setFont(font);
    ui->previewTextEdit->setFont(font);
    ui->decoTextEdit->setFont(font);
}

void MemoryWidget::on_actionHideDisasm_side_panel_triggered()
{
    if (ui->memSideTabWidget_2->isVisible())
    {
        ui->memSideTabWidget_2->hide();
    }
    else
    {
        ui->memSideTabWidget_2->show();
    }
}

void MemoryWidget::on_actionHideHexdump_side_panel_triggered()
{
    if (ui->hexSideTab_2->isVisible())
    {
        ui->hexSideTab_2->hide();
    }
    else
    {
        ui->hexSideTab_2->show();
    }
}

void MemoryWidget::on_actionHideGraph_side_panel_triggered()
{
    if (ui->graphTreeWidget_2->isVisible())
    {
        ui->graphTreeWidget_2->hide();
    }
    else
    {
        ui->graphTreeWidget_2->show();
    }
}

/*
 * Buttons callback functions
 */

void MemoryWidget::on_disButton_2_clicked()
{
    ui->memTabWidget->setCurrentIndex(0);
    ui->memSideTabWidget_2->setCurrentIndex(0);
}

void MemoryWidget::on_hexButton_2_clicked()
{
    ui->memTabWidget->setCurrentIndex(1);
    ui->memSideTabWidget_2->setCurrentIndex(1);
}

void MemoryWidget::on_actionDisas_ShowHideBytes_triggered()
{
    this->main->core->cmd("e!asm.bytes");

    if (this->main->core->cmd("e asm.bytes").trimmed() == "true")
    {
        ui->actionSeparate_bytes->setDisabled(false);
        ui->actionRight_align_bytes->setDisabled(false);
        this->main->core->config("asm.cmtcol", "100");
    }
    else
    {
        ui->actionSeparate_bytes->setDisabled(true);
        ui->actionRight_align_bytes->setDisabled(true);
        this->main->core->config("asm.cmtcol", "70");
    }

    this->refreshDisasm();
}

void MemoryWidget::on_actionDisasSwitch_case_triggered()
{
    this->main->core->cmd("e!asm.ucase");
    this->refreshDisasm();
}

void MemoryWidget::on_actionSyntax_AT_T_Intel_triggered()
{
    QString syntax = this->main->core->cmd("e asm.syntax").trimmed();
    if (syntax == "intel")
    {
        this->main->core->config("asm.syntax", "att");
    }
    else
    {
        this->main->core->config("asm.syntax", "intel");
    }
    this->refreshDisasm();
}

void MemoryWidget::on_actionSeparate_bytes_triggered()
{
    this->main->core->cmd("e!asm.bytespace");
    this->refreshDisasm();
}

void MemoryWidget::on_actionRight_align_bytes_triggered()
{
    this->main->core->cmd("e!asm.lbytes");
    this->refreshDisasm();
}

void MemoryWidget::on_actionSeparate_disasm_calls_triggered()
{
    this->main->core->cmd("e!asm.spacy");
    this->refreshDisasm();
}


void MemoryWidget::on_actionShow_stack_pointer_triggered()
{
    this->main->core->cmd("e!asm.stackptr");
    this->refreshDisasm();
}

void MemoryWidget::on_graphButton_2_clicked()
{
    ui->memTabWidget->setCurrentIndex(2);
    ui->memSideTabWidget_2->setCurrentIndex(0);
}

void MemoryWidget::on_actionSend_to_Notepad_triggered()
{
    QTextCursor cursor = ui->disasTextEdit_2->textCursor();
    QString text = cursor.selectedText();
    this->main->sendToNotepad(text);
}

void MemoryWidget::on_actionDisasAdd_comment_triggered()
{
    // Get current offset
    QTextCursor tc = this->disasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
    if (ele.contains("0x"))
    {
        // Get function for clicked offset
        RAnalFunction *fcn = this->main->core->functionAt(ele.toLongLong(0, 16));
        CommentsDialog *c = new CommentsDialog(this);
        if (c->exec())
        {
            // Get new function name
            QString comment = c->getComment();
            //this->main->add_debug_output("Comment: " + comment + " at: " + ele);
            // Rename function in r2 core
            this->main->core->setComment(ele, comment);
            // Seek to new renamed function
            if (fcn)
            {
                this->main->seek(fcn->name);
            }
            // TODO: Refresh functions tree widget
        }
    }
    this->main->refreshComments();
}

void MemoryWidget::on_actionFunctionsRename_triggered()
{
    // Get current offset
    QTextCursor tc = this->disasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
    if (ele.contains("0x"))
    {
        // Get function for clicked offset
        RAnalFunction *fcn = this->main->core->functionAt(ele.toLongLong(0, 16));
        RenameDialog *r = new RenameDialog(this);
        // Get function based on click position
        r->setFunctionName(fcn->name);
        if (r->exec())
        {
            // Get new function name
            QString new_name = r->getFunctionName();
            // Rename function in r2 core
            this->main->core->renameFunction(fcn->name, new_name);
            // Seek to new renamed function
            this->main->seek(new_name);
            // TODO: Refresh functions tree widget
        }
    }
    this->main->refreshFunctions();
}

void MemoryWidget::on_action8columns_triggered()
{
    this->main->core->config("hex.cols", "8");
    this->refreshHexdump();
}

void MemoryWidget::on_action16columns_triggered()
{
    this->main->core->config("hex.cols", "16");
    this->refreshHexdump();
}

void MemoryWidget::on_action4columns_triggered()
{
    this->main->core->config("hex.cols", "4");
    this->refreshHexdump();
}

void MemoryWidget::on_action32columns_triggered()
{
    this->main->core->config("hex.cols", "32");
    this->refreshHexdump();
}

void MemoryWidget::on_action64columns_triggered()
{
    this->main->core->config("hex.cols", "64");
    this->refreshHexdump();
}

void MemoryWidget::on_action2columns_triggered()
{
    this->main->core->config("hex.cols", "2");
    this->refreshHexdump();
}

void MemoryWidget::on_action1column_triggered()
{
    this->main->core->config("hex.cols", "1");
    this->refreshHexdump();
}

void MemoryWidget::on_xreFromTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    QString offset = item->text(0);
    RAnalFunction *fcn = this->main->core->functionAt(offset.toLongLong(0, 16));
    //this->add_debug_output( fcn->name );
    this->main->seek(offset, fcn->name);
}

void MemoryWidget::on_xrefToTreeWidget_2_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    QString offset = item->text(0);
    RAnalFunction *fcn = this->main->core->functionAt(offset.toLongLong(0, 16));
    //this->add_debug_output( fcn->name );
    this->main->seek(offset, fcn->name);
}

void MemoryWidget::on_xrefFromToolButton_2_clicked()
{
    if (ui->xrefFromToolButton_2->isChecked())
    {
        ui->xreFromTreeWidget_2->hide();
        ui->xrefFromToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xreFromTreeWidget_2->show();
        ui->xrefFromToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_xrefToToolButton_2_clicked()
{
    if (ui->xrefToToolButton_2->isChecked())
    {
        ui->xrefToTreeWidget_2->hide();
        ui->xrefToToolButton_2->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->xrefToTreeWidget_2->show();
        ui->xrefToToolButton_2->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::on_codeCombo_2_currentTextChanged(const QString &arg1)
{
    if (arg1 == "Dissasembly")
    {
        ui->hexSideFrame_2->show();
        ui->hexDisasTextEdit_2->setPlainText(";; Select some bytes on the left\n;; to see them disassembled");
    }
    else
    {
        ui->hexSideFrame_2->hide();
        ui->hexDisasTextEdit_2->setPlainText(";; Select some bytes on the left\n;; to see them parsed here");
    }
}

void MemoryWidget::get_refs_data(const QString &offset)
{
    // Get Refs and Xrefs
    bool ok;
    QList<QStringList> ret_refs;
    QList<QStringList> ret_xrefs;

    // refs = calls q hace esa funcion
    QList<QString> refs = this->main->core->getFunctionRefs(offset.toLong(&ok, 16), 'C');
    if (refs.size() > 0)
    {
        for (int i = 0; i < refs.size(); ++i)
        {
            //this->add_debug_output(refs.at(i));
            QStringList retlist = refs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(2);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_refs << temp;
        }
    }

    // xrefs = calls a esa funcion
    //qDebug() << this->main->core->getFunctionXrefs(offset.toLong(&ok, 16));
    QList<QString> xrefs = this->main->core->getFunctionXrefs(offset.toLong(&ok, 16));
    if (xrefs.size() > 0)
    {
        for (int i = 0; i < xrefs.size(); ++i)
        {
            //this->add_debug_output(xrefs.at(i));
            QStringList retlist = xrefs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(1);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_xrefs << temp;
        }
    }

    // Data for the disasm side graph
    QList<int> data;
    //qDebug() << "Refs:" << refs.size();
    data << refs.size();
    //qDebug() << "XRefs:" << xrefs.size();
    data << xrefs.size();
    //qDebug() << "CC: " << this->main->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    //data << this->main->core->fcnCyclomaticComplexity(offset.toLong(&ok, 16));
    data << this->main->core->getCycloComplex(offset.toLong(&ok, 16));
    //qDebug() << "BB: " << this->main->core->fcnBasicBlockCount(offset.toLong(&ok, 16));
    data << this->main->core->fcnBasicBlockCount(offset.toLong(&ok, 16));
    data << this->main->core->fcnEndBbs(offset);
    //qDebug() << "MEOW: " + this->main->core->fcnEndBbs(offset);

    // Update disasm side bar
    this->fill_refs(ret_refs, ret_xrefs, data);
}

void MemoryWidget::fill_refs(QList<QStringList> refs, QList<QStringList> xrefs, QList<int> graph_data)
{
    this->xreFromTreeWidget_2->clear();
    for (int i = 0; i < refs.size(); ++i)
    {
        //this->add_debug_output(refs.at(i).at(0) + " " + refs.at(i).at(1));
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, refs.at(i).at(0));
        tempItem->setText(1, refs.at(i).at(1));
        tempItem->setToolTip(0, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)).trimmed());
        tempItem->setToolTip(1, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)).trimmed());
        this->xreFromTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count = this->xreFromTreeWidget_2->columnCount();
    for (int i = 0; i != count; ++i)
    {
        this->xreFromTreeWidget_2->resizeColumnToContents(i);
    }

    this->xrefToTreeWidget_2->clear();
    for (int i = 0; i < xrefs.size(); ++i)
    {
        //this->add_debug_output(xrefs.at(i).at(0) + " " + xrefs.at(i).at(1));
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xrefs.at(i).at(0));
        tempItem->setText(1, xrefs.at(i).at(1));
        tempItem->setToolTip(0, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)).trimmed());
        tempItem->setToolTip(1, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)).trimmed());
        this->xrefToTreeWidget_2->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count2 = this->xrefToTreeWidget_2->columnCount();
    for (int i = 0; i != count2; ++i)
    {
        this->xrefToTreeWidget_2->resizeColumnToContents(i);
    }

    // Add data to HTML Polar functions graph
    QFile html(":/html/fcn_graph.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    QString data = QString("\"%1\", \"%2\", \"%3\", \"%4\", \"%5\"").arg(graph_data.at(2)).arg(graph_data.at(0)).arg(graph_data.at(3)).arg(graph_data.at(1)).arg(graph_data.at(4));
    code.replace("MEOW", data);
    ui->fcnWebView->setHtml(code);

    // Add data to HTML Radar functions graph
    QFile html2(":/html/fcn_radar.html");
    if (!html2.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code2 = html2.readAll();
    html2.close();

    QString data2 = QString("%1, %2, %3, %4, %5").arg(graph_data.at(2)).arg(graph_data.at(0)).arg(graph_data.at(3)).arg(graph_data.at(1)).arg(graph_data.at(4));
    code2.replace("MEOW", data2);
    ui->radarGraphWebView->setHtml(code2);
}

void MemoryWidget::fillOffsetInfo(QString off)
{
    ui->offsetTreeWidget->clear();
    QString raw = this->main->core->getOffsetInfo(off);
    QList<QString> lines = raw.split("\n", QString::SkipEmptyParts);
    foreach (QString line, lines)
    {
        QList<QString> eles = line.split(":", QString::SkipEmptyParts);
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, eles.at(0).toUpper());
        tempItem->setText(1, eles.at(1));
        ui->offsetTreeWidget->insertTopLevelItem(0, tempItem);
    }

    // Adjust column to contents
    int count = ui->offsetTreeWidget->columnCount();
    for (int i = 0; i != count; ++i)
    {
        ui->offsetTreeWidget->resizeColumnToContents(i);
    }

    // Add opcode description
    QStringList description = this->main->core->cmd("?d. @ " + off).split(": ");
    if (description.length() >= 2)
    {
        ui->opcodeDescText->setPlainText("# " + description[0] + ":\n" + description[1]);
    }
}

void MemoryWidget::create_graph(QString off)
{
    ui->graphWebView->setZoomFactor(0.85);
    this->main->addDebugOutput("Graph Offset: '" + off + "'");
    if (off == "")
    {
        off = "0x0" + this->main->core->cmd("s").split("0x")[1].trimmed();
    }
    //QString fcn = this->main->core->cmdFunctionAt(off);
    //this->main->add_debug_output("Graph Fcn: " + fcn);
    ui->graphWebView->setUrl(QUrl("qrc:/graph/html/graph/index.html#" + off));
    QString port = this->main->core->config("http.port");
    ui->graphWebView->page()->runJavaScript(QString("r2.root=\"http://localhost:%1\"").arg(port));
    QSettings settings;
    if (settings.value("dark").toBool())
    {
        ui->graphWebView->page()->runJavaScript(QString("init_panel('dark');"));
    } else {
        ui->graphWebView->page()->runJavaScript(QString("init_panel('light');"));
    }
}

QString MemoryWidget::normalize_addr(QString addr)
{
    QString base = this->main->core->cmd("s").split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8)
    {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    }
    else
    {
        return addr.trimmed();
    }
}

void MemoryWidget::setFcnName(RVA addr)
{
    RAnalFunction *fcn;
    QString addr_string;

    fcn = this->main->core->functionAt(addr);
    if (fcn)
    {
        QString segment = this->main->core->cmd("S. @ " + QString::number(addr)).split(" ").last();
        addr_string = segment.trimmed() + ":" + fcn->name;
    }
    else
    {
        addr_string = main->core->cmdFunctionAt(addr);
    }

    ui->fcnNameEdit->setText(addr_string);
}

void MemoryWidget::on_disasTextEdit_2_cursorPositionChanged()
{
    // Get current offset
    QTextCursor tc = this->disasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText().trimmed();
    QList<QString> words = lastline.split(" ", QString::SkipEmptyParts);
    if (words.length() == 0) {
        return;
    }
    QString ele = words[0];
    if (ele.contains("0x"))
    {
        this->fillOffsetInfo(ele);
        QString at = this->main->core->cmdFunctionAt(ele);
        QString deco = this->main->core->getDecompiledCode(at);


        RVA addr = ele.midRef(2).toULongLong(0, 16);
        this->main->setCursorAddress(addr);

        if (deco != "")
        {
            ui->decoTextEdit->setPlainText(deco);
        }
        else
        {
            ui->decoTextEdit->setPlainText("");
        }
        // Get jump information to fill the preview
        QString jump =  this->main->core->getOffsetJump(ele);
        if (!jump.isEmpty())
        {
            // Fill the preview
            QString jump_code = this->main->core->cmd("pdf @ " + jump);
            ui->previewTextEdit->setPlainText(jump_code.trimmed());
            ui->previewTextEdit->moveCursor(QTextCursor::End);
            ui->previewTextEdit->find(jump.trimmed(), QTextDocument::FindBackward);
            ui->previewTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        }
        else
        {
            ui->previewTextEdit->setPlainText("");
        }
        //this->main->add_debug_output("Fcn at: '" + at + "'");
        if (this->last_fcn != at)
        {
            this->last_fcn = at;
            //this->main->add_debug_output("New Fcn: '" + this->last_fcn + "'");
            // Refresh function information at sidebar
            ui->fcnNameEdit->setText(at);
            this->main->memoryDock->setWindowTitle(at);
            this->main->memoryDock->get_refs_data(ele);
            //this->main->memoryDock->create_graph(ele);
            this->setMiniGraph(at);
        }
    }
}

QString MemoryWidget::normalizeAddr(QString addr)
{
    QString base = addr.split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8)
    {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    }
    else
    {
        return addr;
    }
}

void MemoryWidget::setMiniGraph(QString at)
{
    QString dot = this->main->core->getSimpleGraph(at);
    //QString dot = this->main->core->cmd("agc " + at);
    // Add data to HTML Polar functions graph
    QFile html(":/html/graph.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    code.replace("MEOW", dot);
    ui->webSimpleGraph->setHtml(code);

}

void MemoryWidget::on_polarToolButton_clicked()
{
    ui->radarToolButton->setChecked(false);
    ui->fcnGraphTabWidget->setCurrentIndex(0);
}

void MemoryWidget::on_radarToolButton_clicked()
{
    ui->polarToolButton->setChecked(false);
    ui->fcnGraphTabWidget->setCurrentIndex(1);
}


void MemoryWidget::on_hexSideTab_2_currentChanged(int /*index*/)
{
    /*
    if (index == 2) {
        // Add data to HTML Polar functions graph
        QFile html(":/html/bar.html");
        if(!html.open(QIODevice::ReadOnly)) {
            QMessageBox::information(0,"error",html.errorString());
        }
        QString code = html.readAll();
        html.close();
        this->histoWebView->setHtml(code);
        this->histoWebView->show();
    } else {
        this->histoWebView->hide();
    }
    */
}

void MemoryWidget::on_memSideToolButton_clicked()
{
    if (ui->memSideToolButton->isChecked())
    {
        ui->memSideTabWidget_2->hide();
        ui->hexSideTab_2->hide();
        ui->memSideToolButton->setIcon(QIcon(":/new/prefix1/img/icons/left.png"));
    }
    else
    {
        ui->memSideTabWidget_2->show();
        ui->hexSideTab_2->show();
        ui->memSideToolButton->setIcon(QIcon(":/new/prefix1/img/icons/right.png"));
    }
}

void MemoryWidget::on_previewToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(0);
}

void MemoryWidget::on_decoToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(1);
}

void MemoryWidget::on_simpleGrapgToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(2);
}

void MemoryWidget::on_previewToolButton_2_clicked()
{
    if (ui->previewToolButton_2->isChecked())
    {
        ui->frame_3->setVisible(true);
    }
    else
    {
        ui->frame_3->setVisible(false);
    }
}

void MemoryWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible())
    {
        if (event->size().width() <= 1150)
        {
            ui->frame_3->setVisible(false);
            ui->memPreviewTab->setVisible(false);
            ui->previewToolButton_2->setChecked(false);
            if (event->size().width() <= 950)
            {
                ui->memSideTabWidget_2->hide();
                ui->hexSideTab_2->hide();
                ui->memSideToolButton->setChecked(true);
            }
            else
            {
                ui->memSideTabWidget_2->show();
                ui->hexSideTab_2->show();
                ui->memSideToolButton->setChecked(false);
            }
        }
        else
        {
            ui->frame_3->setVisible(true);
            ui->memPreviewTab->setVisible(true);
            ui->previewToolButton_2->setChecked(true);
        }
    }
    QDockWidget::resizeEvent(event);
}

bool MemoryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == ui->disasTextEdit_2 || obj == ui->disasTextEdit_2->viewport()) && event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        //qDebug()<<QString("Click location: (%1,%2)").arg(mouseEvent->x()).arg(mouseEvent->y());
        QTextCursor cursor = ui->disasTextEdit_2->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        cursor.select(QTextCursor::LineUnderCursor);
        QString lastline = cursor.selectedText();
        auto eles = lastline.split(" ", QString::SkipEmptyParts);
        QString ele = eles.isEmpty() ? "" : eles[0];
        if (ele.contains("0x"))
        {
            QString jump = this->main->core->getOffsetJump(ele);
            if (!jump.isEmpty())
            {
                if (jump.contains("0x"))
                {
                    QString fcn = this->main->core->cmdFunctionAt(jump);
                    if (!fcn.isEmpty())
                    {
                        this->main->seek(jump.trimmed(), fcn);
                    }
                }
                else
                {
                    this->main->seek(this->main->core->cmd("?v " + jump), jump);
                }
            }
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void MemoryWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->xreFromTreeWidget_2);
    qhelpers::setVerticalScrollMode(ui->xrefToTreeWidget_2);
}

void MemoryWidget::on_actionXRefs_triggered()
{
    // Get current offset
    QTextCursor tc = this->disasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
    if (ele.contains("0x"))
    {
        // Get function for clicked offset
        RAnalFunction *fcn = this->main->core->functionAt(ele.toLongLong(0, 16));
        if (!fcn)
        {
            return;
        }
        XrefsDialog *x = new XrefsDialog(this->main, this);
        x->setWindowTitle("X-Refs for function " + QString(fcn->name));
        x->updateLabels(QString(fcn->name));

        // Get Refs and Xrefs
        QList<QStringList> ret_refs;
        QList<QStringList> ret_xrefs;

        // refs = calls q hace esa funcion
        QList<QString> refs = this->main->core->getFunctionRefs(fcn->addr, 'C');
        if (refs.size() > 0)
        {
            for (int i = 0; i < refs.size(); ++i)
            {
                //this->main->add_debug_output(refs.at(i));
                QStringList retlist = refs.at(i).split(",");
                QStringList temp;
                QString addr = retlist.at(2);
                temp << addr;
                QString op = this->main->core->cmd("pi 1 @ " + addr);
                temp << op.simplified();
                ret_refs << temp;
            }
        }

        // xrefs = calls a esa funcion
        //qDebug() << this->main->core->getFunctionXrefs(offset.toLong(&ok, 16));
        QList<QString> xrefs = this->main->core->getFunctionXrefs(fcn->addr);
        if (xrefs.size() > 0)
        {
            for (int i = 0; i < xrefs.size(); ++i)
            {
                //this->main->add_debug_output(xrefs.at(i));
                QStringList retlist = xrefs.at(i).split(",");
                QStringList temp;
                QString addr = retlist.at(1);
                temp << addr;
                QString op = this->main->core->cmd("pi 1 @ " + addr);
                temp << op.simplified();
                ret_xrefs << temp;
            }
        }
        x->fillRefs(ret_refs, ret_xrefs);
        x->exec();
    }
}


void MemoryWidget::on_copyMD5_clicked()
{
    QString md5 = ui->bytesMD5->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(md5);
    this->main->addOutput("MD5 copied to clipboard: " + md5);
}

void MemoryWidget::on_copySHA1_clicked()
{
    QString sha1 = ui->bytesSHA1->text();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(sha1);
    this->main->addOutput("SHA1 copied to clipboard: " + sha1);
}

void MemoryWidget::switchTheme(bool dark)
{
    if (dark)
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(64, 64, 64));
        ui->graphWebView->page()->runJavaScript("r2ui.graph_panel.render('dark');");
    }
    else
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(255, 255, 255));
        ui->graphWebView->page()->runJavaScript("r2ui.graph_panel.render('light');");
    }
}

void MemoryWidget::on_opcodeDescButton_clicked()
{
    if (ui->opcodeDescButton->isChecked())
    {
        ui->opcodeDescText->hide();
        ui->opcodeDescButton->setArrowType(Qt::RightArrow);
    }
    else
    {
        ui->opcodeDescText->show();
        ui->opcodeDescButton->setArrowType(Qt::DownArrow);
    }
}

void MemoryWidget::selectHexPreview()
{
    // Pre-select arch and bits in the hexdump sidebar
    QString arch = this->main->core->cmd("e asm.arch").trimmed();
    QString bits = this->main->core->cmd("e asm.bits").trimmed();

    //int arch_index = ui->hexArchComboBox_2->findText(arch);
    if (ui->hexArchComboBox_2->findText(arch) != -1)
    {
        ui->hexArchComboBox_2->setCurrentIndex(ui->hexArchComboBox_2->findText(arch));
    }

    //int bits_index = ui->hexBitsComboBox_2->findText(bits);
    if (ui->hexBitsComboBox_2->findText(bits) != -1)
    {
        ui->hexBitsComboBox_2->setCurrentIndex(ui->hexBitsComboBox_2->findText(bits));
    }
}

void MemoryWidget::seek_back()
{
    //this->main->add_debug_output("Back!");
    this->main->on_backButton_clicked();
}

void MemoryWidget::frameLoadFinished(bool ok)
{
    //qDebug() << "LOAD FRAME: " << ok;
    if (ok)
    {
        QSettings settings;
        if (settings.value("dark").toBool())
        {
            QString js = "r2ui.graph_panel.render('dark');";
            ui->graphWebView->page()->runJavaScript(js);
        }
    }
}

void MemoryWidget::on_memTabWidget_currentChanged(int /*index*/)
{
    /*this->main->add_debug_output("Update index: " + QString::number(index) + " to function: " + RAddressString(main->getCursorAddress()));
    this->main->add_debug_output("Last disasm: " + RAddressString(this->last_disasm_fcn));
    this->main->add_debug_output("Last graph: " + RAddressString(this->last_graph_fcn));
    this->main->add_debug_output("Last hexdump: " + RAddressString(this->last_hexdump_fcn));*/
    this->updateViews();
}

void MemoryWidget::updateViews()
{
    // Update only the selected view to improve performance

    int index = ui->memTabWidget->tabBar()->currentIndex();

    RVA cursor_addr = main->getCursorAddress();

    QString cursor_addr_string = RAddressString(cursor_addr);

    if (index == 0)
    {
        // Disasm
        if (this->last_disasm_fcn != cursor_addr)
        {
            this->refreshDisasm(cursor_addr_string);
            this->last_disasm_fcn = cursor_addr;
        }
    }
    else if (index == 1)
    {
        // Hex
        if (this->last_hexdump_fcn != cursor_addr)
        {
            this->refreshHexdump(cursor_addr_string);
            this->last_hexdump_fcn = cursor_addr;
        }
    }
    else if (index == 2)
    {
        // Graph
        if (this->last_graph_fcn != cursor_addr)
        {
            this->create_graph(cursor_addr_string);
            this->last_graph_fcn = cursor_addr;
        }
    }
}
