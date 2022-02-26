#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QDialogButtonBox>


class About : public QDialog
{
    Q_OBJECT

public:
    About(QWidget *parent = nullptr);
    About(const QString titel, QWidget *parent = nullptr);

    void addCredit(QString text);
    void addCredits(QStringList credits);
    void deleteCreditPage();
    void setAppUrl(QUrl url);
    void setAppUrl(QString url);

private:
    void setupUi();
    void retranslateUi();

    bool creditPage = true;
    QString homepage;

    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QFormLayout *formLayout;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_6;
    QLabel *label_7;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_8;
    QSpacerItem *verticalSpacer;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_2;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_5;
    QDialogButtonBox *buttonBox;
};

#endif // ABOUT_H
