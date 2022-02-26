#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
QT_END_NAMESPACE

class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    TextEdit(QWidget *parent = nullptr);

    bool load(const QString &f);

    void language(QString language);

public slots:
    void fileNew();

protected:
    void closeEvent(QCloseEvent *e) override;

private slots:
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QFont &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void textColor();
    void textAlign(QAction *a);

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();
    void about();
    void printPreview(QPrinter *);

    void retranslateUi();

private:
    void setupFileActions();
    void setupEditActions();
    void setupTextActions();
    bool maybeSave();
    void setCurrentFileName(const QString &fileName);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    QSettings *settings;
    void loadSettings();
    void saveSettings();

    QToolBar *tbFile;
    QToolBar *tbEdit;
    QToolBar *tbFormat;
    QToolBar *tbFormatActions;

    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuFormat;
    QMenu *menuHelp;

    QAction *actionNew;
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionSaveAs;
    QAction *actionPrint;
    QAction *actionPrintPreview;
    QAction *actionExportPdf;
    QAction *actionExit;
    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;
    QAction *actionTextColor;
    QAction *actionAlignLeft;
    QAction *actionAlignCenter;
    QAction *actionAlignRight;
    QAction *actionAlignJustify;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionSelectAll;
#ifndef QT_NO_CLIPBOARD
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
#endif
    QAction *actionAbout;
    QAction *actionAboutQt;

    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;

    QString fileName;
    QTextEdit *textEdit;

signals:
    void languageChanged();
};

#endif // TEXTEDIT_H
