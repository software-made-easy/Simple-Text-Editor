#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QActionGroup>
#include <QTranslator>
#include <QLocale>
#include <QStandardPaths>
#include <QScreen>

#include "textedit.h"
#include "about.h"


#if defined(Q_OS_BLACKBERRY) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_WP)
#define Q_OS_MOBILE
#else
#define Q_OS_DESKTOP
#endif
#if (defined (LINUX) || defined (__linux__)) && !(defined (ANDROID) || defined (__ANDROID__))
#define JUST_LINUX
#endif

QString rsrcPath(":/images");
QTranslator translator, qtTranslator;


TextEdit::TextEdit(QWidget *parent)
    : QMainWindow(parent)
{
#ifdef Q_OS_OSX
    setUnifiedTitleAndToolBarOnMac(true);
#endif
    // setWindowTitle(QCoreApplication::applicationName());
    connect(this, &TextEdit::languageChanged, this, &TextEdit::retranslateUi);
    setWindowIcon(QIcon(":/Icon.png"));

    textEdit = new QTextEdit(this);
    connect(textEdit, &QTextEdit::currentCharFormatChanged,
            this, &TextEdit::currentCharFormatChanged);
    connect(textEdit, &QTextEdit::cursorPositionChanged,
            this, &TextEdit::cursorPositionChanged);
    setCentralWidget(textEdit);

    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setupFileActions();
    setupEditActions();
    setupTextActions();

    menuHelp = menuBar()->addMenu(tr("Help"));
    actionAbout = menuHelp->addAction(tr("About"), this, &TextEdit::about);
    actionAboutQt = menuHelp->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);


    QFont textFont;
    textFont.setStyleHint(QFont::SansSerif);
    textEdit->setFont(textFont);
    fontChanged(textEdit->font());
    colorChanged(textEdit->textColor());
    alignmentChanged(textEdit->alignment());

    connect(textEdit->document(), &QTextDocument::modificationChanged,
            actionSave, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::modificationChanged,
            this, &QWidget::setWindowModified);
    connect(textEdit->document(), &QTextDocument::undoAvailable,
            actionUndo, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::redoAvailable,
            actionRedo, &QAction::setEnabled);

    setWindowModified(textEdit->document()->isModified());
    actionSave->setEnabled(textEdit->document()->isModified());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

#ifndef QT_NO_CLIPBOARD
    actionCut->setEnabled(false);
    connect(textEdit, &QTextEdit::copyAvailable, actionCut, &QAction::setEnabled);
    actionCopy->setEnabled(false);
    connect(textEdit, &QTextEdit::copyAvailable, actionCopy, &QAction::setEnabled);

    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &TextEdit::clipboardDataChanged);
#endif

    textEdit->setFocus();
    setCurrentFileName(QString());

#ifdef Q_OS_MACOS
    // Use dark text on light background on macOS, also in dark mode.
    QPalette pal = textEdit->palette();
    pal.setColor(QPalette::Base, QColor(Qt::white));
    pal.setColor(QPalette::Text, QColor(Qt::black));
    textEdit->setPalette(pal);
#endif
#ifdef Q_OS_ANDROID
    settings = new QSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/SME_STE.ini", QSettings::IniFormat);
#else
    settings = new QSettings("SME", "STE", this);
#endif
    loadSettings();
}

void TextEdit::closeEvent(QCloseEvent *e)
{
    if (maybeSave()) {
        e->accept();
        saveSettings();
    }
    else
        e->ignore();
}

void TextEdit::setupFileActions()
{
    tbFile = addToolBar(tr("File Actions"));
    tbFile->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tbFile->setObjectName("File_Toolbar");
    menuFile = menuBar()->addMenu(tr("&File"));

    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(rsrcPath + "/document-new.svg"));
    actionNew = menuFile->addAction(newIcon,  tr("&New"), this, &TextEdit::fileNew, QKeySequence::New);
    tbFile->addAction(actionNew);

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(rsrcPath + "/document-open.svg"));
    actionOpen = menuFile->addAction(openIcon, tr("&Open..."), this, &TextEdit::fileOpen, QKeySequence::Open);
    tbFile->addAction(actionOpen);

    menuFile->addSeparator();

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(rsrcPath + "/document-save.svg"));
    actionSave = menuFile->addAction(saveIcon, tr("&Save"), this, &TextEdit::fileSave, QKeySequence::Save);
    actionSave->setEnabled(false);
    tbFile->addAction(actionSave);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as", QIcon(rsrcPath + "/document-save-as.svg"));
    actionSaveAs = menuFile->addAction(saveAsIcon, tr("Save &As"), this, &TextEdit::fileSaveAs, QKeySequence("Ctrl+Shift+S"));
    menuFile->addSeparator();

#ifndef QT_NO_PRINTER
    const QIcon printIcon = QIcon::fromTheme("document-print", QIcon(rsrcPath + "/document-print.svg"));
    actionPrint = menuFile->addAction(printIcon, tr("&Print"), this, &TextEdit::filePrint, QKeySequence::Print);
    tbFile->addAction(actionPrint);

    const QIcon filePrintIcon = QIcon::fromTheme("document-print-preview", QIcon(rsrcPath + "/document-print-preview.svg"));
    actionPrintPreview = menuFile->addAction(filePrintIcon, tr("Print Preview"), this, &TextEdit::filePrintPreview);

    QIcon exportPdfIcon;
#ifdef JUST_LINUX
    exportPdfIcon = QIcon(rsrcPath + "/export-pdf-dark.svg");
#else
    exportPdfIcon = QIcon(rsrcPath + "/export-pdf.svg");
#endif
    actionExportPdf = menuFile->addAction(exportPdfIcon, tr("&Export PDF"), this, &TextEdit::filePrintPdf, QKeySequence("Ctrl+D"));

#ifndef Q_OS_MOBILE
    tbFile->addAction(actionExportPdf);
#endif

    menuFile->addSeparator();
#endif

    const QIcon exitIcon = QIcon::fromTheme("application-exit", QIcon(rsrcPath + "/application-exit.svg"));
    actionExit = menuFile->addAction(exitIcon, tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);
}

void TextEdit::setupEditActions()
{
    tbEdit = addToolBar(tr("Edit Actions"));
    tbEdit->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tbEdit->setObjectName("Edit_Toolbar");
    menuEdit = menuBar()->addMenu(tr("&Edit"));

    const QIcon undoIcon = QIcon::fromTheme("edit-undo", QIcon(rsrcPath + "/edit-undo.svg"));
    actionUndo = menuEdit->addAction(undoIcon, tr("&Undo"), textEdit, &QTextEdit::undo, QKeySequence::Undo);
    tbEdit->addAction(actionUndo);

    const QIcon redoIcon = QIcon::fromTheme("edit-redo", QIcon(rsrcPath + "/edit-redo.svg"));
    actionRedo = menuEdit->addAction(redoIcon, tr("&Redo"), textEdit, &QTextEdit::redo, QKeySequence::Redo);
    tbEdit->addAction(actionRedo);
    menuEdit->addSeparator();

#ifndef QT_NO_CLIPBOARD
    const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(rsrcPath + "/edit-cut.svg"));
    actionCut = menuEdit->addAction(cutIcon, tr("Cu&t"), textEdit, &QTextEdit::cut, QKeySequence::Cut);
    tbEdit->addAction(actionCut);

    const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(rsrcPath + "/edit-copy.svg"));
    actionCopy = menuEdit->addAction(copyIcon, tr("&Copy"), textEdit, &QTextEdit::copy, QKeySequence::Copy);
    tbEdit->addAction(actionCopy);

    const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(rsrcPath + "/edit-paste.svg"));
    actionPaste = menuEdit->addAction(pasteIcon, tr("&Paste"), textEdit, &QTextEdit::paste, QKeySequence::Paste);
    tbEdit->addAction(actionPaste);
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::setupTextActions()
{
    tbFormat = addToolBar(tr("Format Actions"));
    tbFormat->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tbFormat->setObjectName("Format_Toolbar");
    menuFormat = menuBar()->addMenu(tr("F&ormat"));

    const QIcon boldIcon = QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/format-text-bold.svg"));
    actionTextBold = menuFormat->addAction(boldIcon, tr("&Bold"), this, &TextEdit::textBold, Qt::CTRL | Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    tbFormat->addAction(actionTextBold);
    actionTextBold->setCheckable(true);

    const QIcon italicIcon = QIcon::fromTheme("format-text-italic", QIcon(rsrcPath + "/format-text-italic.svg"));
    actionTextItalic = menuFormat->addAction(italicIcon, tr("&Italic"), this, &TextEdit::textItalic, Qt::CTRL | Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    tbFormat->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);

    const QIcon underlineIcon = QIcon::fromTheme("format-text-underline", QIcon(rsrcPath + "/format-text-underline.svg"));
    actionTextUnderline = menuFormat->addAction(underlineIcon, tr("&Underline"), this, &TextEdit::textUnderline, Qt::CTRL | Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    tbFormat->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);

    menuFormat->addSeparator();

    const QIcon leftIcon = QIcon::fromTheme("format-justify-left", QIcon(rsrcPath + "/format-justify-left.svg"));
    actionAlignLeft = new QAction(leftIcon, tr("&Left"), this);
    actionAlignLeft->setShortcut(Qt::CTRL | Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    const QIcon centerIcon = QIcon::fromTheme("format-justify-center", QIcon(rsrcPath + "/format-justify-center.svg"));
    actionAlignCenter = new QAction(centerIcon, tr("C&enter"), this);
    actionAlignCenter->setShortcut(Qt::CTRL | Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    const QIcon rightIcon = QIcon::fromTheme("format-justify-right", QIcon(rsrcPath + "/format-justify-right.svg"));
    actionAlignRight = new QAction(rightIcon, tr("&Right"), this);
    actionAlignRight->setShortcut(Qt::CTRL | Qt::Key_R);
    actionAlignRight->setCheckable(true);
    const QIcon fillIcon = QIcon::fromTheme("format-justify-fill", QIcon(rsrcPath + "/format-justify-fill.svg"));
    actionAlignJustify = new QAction(fillIcon, tr("&Justify"), this);
    actionAlignJustify->setShortcut(Qt::CTRL | Qt::Key_J);
    actionAlignJustify->setCheckable(true);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(this);
    connect(alignGroup, &QActionGroup::triggered, this, &TextEdit::textAlign);

    if (QApplication::isLeftToRight()) {
        alignGroup->addAction(actionAlignLeft);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignRight);
    } else {
        alignGroup->addAction(actionAlignRight);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignLeft);
    }
    alignGroup->addAction(actionAlignJustify);

    tbFormat->addActions(alignGroup->actions());
    menuFormat->addActions(alignGroup->actions());

    menuFormat->addSeparator();

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    actionTextColor = menuFormat->addAction(pix, tr("&Color..."), this, &TextEdit::textColor);
    tbFormat->addAction(actionTextColor);

    tbFormatActions = addToolBar(tr("Format Actions"));
    tbFormatActions->setObjectName("Formar_Actions_Toolbar");
    tbFormatActions->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(tbFormatActions);

    comboStyle = new QComboBox(tbFormatActions);
    tbFormatActions->addWidget(comboStyle);
    comboStyle->addItem("Standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    comboStyle->addItem("Ordered List (Roman lower)");
    comboStyle->addItem("Ordered List (Roman upper)");
    comboStyle->addItem("Heading 1");
    comboStyle->addItem("Heading 2");
    comboStyle->addItem("Heading 3");
    comboStyle->addItem("Heading 4");
    comboStyle->addItem("Heading 5");
    comboStyle->addItem("Heading 6");

    connect(comboStyle, QOverload<int>::of(&QComboBox::activated), this, &TextEdit::textStyle);

    comboFont = new QFontComboBox(tbFormatActions);
    tbFormatActions->addWidget(comboFont);
    connect(comboFont, QOverload<const QFont &>::of(&QFontComboBox::currentFontChanged), this, &TextEdit::textFamily);

    comboSize = new QComboBox(tbFormatActions);
    comboSize->setObjectName("comboSize");
    tbFormatActions->addWidget(comboSize);
    comboSize->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    foreach (int size, standardSizes)
        comboSize->addItem(QString::number(size));
    comboSize->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));

    connect(comboSize, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &TextEdit::textSize);
}

bool TextEdit::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        textEdit->setHtml(str);
    }
    else {
        str = QString::fromLocal8Bit(data);
        textEdit->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}

bool TextEdit::maybeSave()
{
    if (!textEdit->document()->isModified())
        return true;

    const int ret =
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("The document has been modified.\n"
                                "Do you want to save your changes?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void TextEdit::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    textEdit->document()->setModified(false);

    QString shownName;
    if (fileName.isEmpty())
        shownName = tr("untitled.txt");
    else
        shownName = QFileInfo(fileName).fileName();

    setWindowTitle(tr("%1[*]").arg(shownName));
    setWindowModified(false);
}

void TextEdit::fileNew()
{
    if (maybeSave()) {
        textEdit->clear();
        setCurrentFileName(QString());
    }
}

void TextEdit::fileOpen()
{
    QFileDialog fileDialog(this, tr("Open File..."));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setMimeTypeFilters(QStringList() << "text/html" << "text/plain");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    QStringList files = fileDialog.selectedFiles();
    const QString fn = files.first();
    if (!load(fn))
        statusBar()->showMessage(tr("Could not open \"%1\"").arg(QDir::toNativeSeparators(fn)));
}

bool TextEdit::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();
    if (fileName.startsWith(QStringLiteral(":/")))
        return fileSaveAs();

    QTextDocumentWriter writer(fileName);
    bool success = writer.write(textEdit->document());
    if (success) {
        textEdit->document()->setModified(false);
        statusBar()->showMessage(tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    } else {
        statusBar()->showMessage(tr("Could not write to file \"%1\"")
                                 .arg(QDir::toNativeSeparators(fileName)));
    }
    return success;
}

bool TextEdit::fileSaveAs()
{
    QFileDialog fileDialog(this, tr("Save as..."));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    QStringList mimeTypes;
    mimeTypes << "application/vnd.oasis.opendocument.text" << "text/html" << "text/plain";
    fileDialog.setMimeTypeFilters(mimeTypes);
    fileDialog.setDefaultSuffix("odt");
    if (fileDialog.exec() != QDialog::Accepted)
        return false;
    QStringList files = fileDialog.selectedFiles();
    const QString fn = files.first();
    setCurrentFileName(fn);
    return fileSave();
}

void TextEdit::filePrint()
{
#if QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    /*if (textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);*/
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        textEdit->print(&printer);
    delete dlg;
#endif
}

void TextEdit::filePrintPreview()
{
#if QT_CONFIG(printpreviewdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &TextEdit::printPreview);
    preview.exec();
#endif
}

void TextEdit::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    textEdit->print(printer);
#endif
}

void TextEdit::filePrintPdf()
{
#ifndef QT_NO_PRINTER
//! [0]
    QFileDialog fileDialog(this, tr("Export PDF"));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setMimeTypeFilters(QStringList("application/pdf"));
    fileDialog.setDefaultSuffix("pdf");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    QStringList files = fileDialog.selectedFiles();
    QString fileName = files.first();
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    textEdit->document()->print(&printer);
    statusBar()->showMessage(tr("Exported \"%1\"")
                             .arg(QDir::toNativeSeparators(fileName)));
//! [0]
#endif
}

void TextEdit::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textFamily(const QFont &f)
{
    QTextCharFormat fmt;
    fmt.setFont(f);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textSize(const QString &p)
{
    qreal pointSize = p.toInt();

    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void TextEdit::textStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();
    QTextListFormat::Style style = QTextListFormat::ListStyleUndefined;

    switch (styleIndex) {
    case 1:
        style = QTextListFormat::ListDisc;
        break;
    case 2:
        style = QTextListFormat::ListCircle;
        break;
    case 3:
        style = QTextListFormat::ListSquare;
        break;
    case 4:
        style = QTextListFormat::ListDecimal;
        break;
    case 5:
        style = QTextListFormat::ListLowerAlpha;
        break;
    case 6:
        style = QTextListFormat::ListUpperAlpha;
        break;
    case 7:
        style = QTextListFormat::ListLowerRoman;
        break;
    case 8:
        style = QTextListFormat::ListUpperRoman;
        break;
    default:
        break;
    }

    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    if (style == QTextListFormat::ListStyleUndefined) {
        blockFmt.setObjectIndex(-1);
        int headingLevel = styleIndex >= 9 ? styleIndex - 9 + 1 : 0; // H1 to H6, or Standard
        blockFmt.setHeadingLevel(headingLevel);
        cursor.setBlockFormat(blockFmt);

        int sizeAdjustment = headingLevel ? 4 - headingLevel : 0; // H1 to H6: +3 to -2
        QTextCharFormat fmt;
        fmt.setFontWeight(headingLevel ? QFont::Bold : QFont::Normal);
        fmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.mergeCharFormat(fmt);
        textEdit->mergeCurrentCharFormat(fmt);
    } else {
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}

void TextEdit::textColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void TextEdit::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == actionAlignCenter)
        textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == actionAlignJustify)
        textEdit->setAlignment(Qt::AlignJustify);
}

void TextEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void TextEdit::cursorPositionChanged()
{
    alignmentChanged(textEdit->alignment());
    QTextList *list = textEdit->textCursor().currentList();
    if (list) {
        switch (list->format().style()) {
        case QTextListFormat::ListDisc:
            comboStyle->setCurrentIndex(1);
            break;
        case QTextListFormat::ListCircle:
            comboStyle->setCurrentIndex(2);
            break;
        case QTextListFormat::ListSquare:
            comboStyle->setCurrentIndex(3);
            break;
        case QTextListFormat::ListDecimal:
            comboStyle->setCurrentIndex(4);
            break;
        case QTextListFormat::ListLowerAlpha:
            comboStyle->setCurrentIndex(5);
            break;
        case QTextListFormat::ListUpperAlpha:
            comboStyle->setCurrentIndex(6);
            break;
        case QTextListFormat::ListLowerRoman:
            comboStyle->setCurrentIndex(7);
            break;
        case QTextListFormat::ListUpperRoman:
            comboStyle->setCurrentIndex(8);
            break;
        default:
            comboStyle->setCurrentIndex(-1);
            break;
        }
    } else {
        int headingLevel = textEdit->textCursor().blockFormat().headingLevel();
        comboStyle->setCurrentIndex(headingLevel ? headingLevel + 8 : 0);
    }
}

void TextEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::about()
{
    About dialog(tr("About Simple Text Editor"), this);
    dialog.setAppUrl("https://github.com/software-made-easy/Simple-Text-Editor");
    dialog.exec();
}

void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection()) {
        return;
        // cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

void TextEdit::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}

void TextEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}

void TextEdit::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        actionAlignJustify->setChecked(true);
}

void TextEdit::retranslateUi() {
    menuFile->setTitle(tr("&File"));
    actionNew->setText(tr("&New"));
    actionOpen->setText(tr("&Open..."));
    actionSave->setText(tr("&Save"));
    actionSaveAs->setText(tr("Save &As"));
    actionPrint->setText(tr("&Print"));
    actionPrintPreview->setText(tr("Print Preview"));
    actionExportPdf->setText(tr("&Export PDF"));
    actionExit->setText(tr("&Quit"));

    menuEdit->setTitle(tr("&Edit"));
    actionUndo->setText(tr("&Undo"));
    actionRedo->setText(tr("&Redo"));
    actionCut->setText(tr("Cu&t"));
    actionCopy->setText(tr("&Copy"));
    actionPaste->setText(tr("&Paste"));

    menuFormat->setTitle(tr("F&ormat"));
    actionTextBold->setText(tr("&Bold"));
    actionTextItalic->setText(tr("&Italic"));
    actionTextUnderline->setText(tr("&Underline"));
    actionAlignLeft->setText(tr("&Left"));
    actionAlignCenter->setText(tr("C&enter"));
    actionAlignRight->setText(tr("&Right"));
    actionAlignJustify->setText(tr("&Justify"));
    actionTextColor->setText(tr("&Color..."));

    menuHelp->setTitle(tr("Help"));
    actionAbout->setText(tr("About"));
    actionAboutQt->setText(tr("About &Qt"));
}

void TextEdit::language(QString lang) {
    if (lang != "en") {
        if (translator.load(":/i18n/STE_" + lang)) {
            qApp->installTranslator(&translator);
        }

        if (qtTranslator.load(":/translations/qtbase_" + lang)) {
            qApp->installTranslator(&qtTranslator);
        }
        settings->setValue("lang", lang);
        emit languageChanged();
    }
    else {
        qApp->removeTranslator(&translator);
        qApp->removeTranslator(&qtTranslator);
    }
}

void TextEdit::loadSettings() {
    const QByteArray geo = settings->value("geometry", QByteArray({})).toByteArray();
    if (geo.isEmpty()) {
        const QRect availableGeometry = qApp->screenAt(pos())->availableGeometry();
        resize(availableGeometry.width() / 2, (availableGeometry.height() * 2) / 3);
        move((availableGeometry.width() - width()) / 2,
                    (availableGeometry.height() - height()) / 2);
    }
    else {
        restoreGeometry(geo);
    }
    restoreState(settings->value("state", QByteArray({})).toByteArray());

    QString lang = settings->value("lang", QString("")).toString();
    if (lang.isEmpty()) {
        lang = QLocale::system().name().split("_").first();
    }
    if (lang != "en") {
        language(lang);
    }
}

void TextEdit::saveSettings() {
    settings->setValue("geometry", saveGeometry());
    settings->setValue("state", saveState());
    settings->sync();
}
