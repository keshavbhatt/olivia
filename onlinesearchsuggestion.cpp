#include "onlinesearchsuggestion.h"
#include <QSettings>

const QString suggestUrl(QStringLiteral("http://suggestqueries.google.com/complete/search?ds=yt&client=youtube&hjson=t&cp=1&format=5&alt=json&q=%1"));


onlineSearchSuggestion::onlineSearchSuggestion(QLineEdit *parent): QObject(parent), editor(parent)
{
    blacklist<<"whatsapp status"<<"full movie"<<"download"<<"movie"<<"movies"<<"lyrics"<<"lyric";

    popup = new QTreeWidget;
    popup->setWindowFlags(Qt::Popup);
    popup->setFocusPolicy(Qt::NoFocus);
    popup->setFocusProxy(parent);
    popup->setMouseTracking(true);

    popup->setColumnCount(1);
    popup->setUniformRowHeights(true);
    popup->setRootIsDecorated(false);
    popup->setEditTriggers(QTreeWidget::NoEditTriggers);
    popup->setSelectionBehavior(QTreeWidget::SelectRows);
    popup->setFrameStyle(QFrame::Box | QFrame::Plain);
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    popup->header()->hide();
    popup->setStyleSheet("border:none;");

    popup->installEventFilter(this);

    connect(popup, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            SLOT(doneCompletion()));

    timer.setSingleShot(true);
    timer.setInterval(500);
    connect(&timer, SIGNAL(timeout()), SLOT(autoSuggest()));
    connect(editor, SIGNAL(textEdited(QString)), &timer, SLOT(start()));

    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(handleNetworkData(QNetworkReply*)));
}


onlineSearchSuggestion::~onlineSearchSuggestion()
{
    delete popup;
}

bool onlineSearchSuggestion::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != popup)
        return false;

    if (ev->type() == QEvent::MouseButtonPress) {
        popup->hide();
        editor->setFocus();
        return true;
    }

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            doneCompletion();
            consumed = true;
            break;

        case Qt::Key_Escape:
            editor->setFocus();
            popup->hide();
            consumed = true;
            break;

        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            break;

        default:
            editor->setFocus();
            editor->event(ev);
            popup->hide();
            break;
        }

        return consumed;
    }

    return false;
}

void onlineSearchSuggestion::showCompletion(const QVector<QString> &choices)
{
    if (choices.isEmpty())
        return;

    const QPalette &pal = editor->palette();
    QColor color = pal.color(QPalette::Normal, QPalette::WindowText);

    popup->setUpdatesEnabled(false);
    popup->clear();

    for (const auto &choice : choices) {
        auto item  = new QTreeWidgetItem(popup);
        item->setText(0, choice);
        item->setTextColor(0, color);
    }

    popup->setCurrentItem(popup->topLevelItem(0));
    popup->resizeColumnToContents(0);
    popup->setUpdatesEnabled(true);

    popup->move(editor->mapToGlobal(QPoint(0, editor->height())));
    popup->setFocus();
    if(editor->hasFocus()||!editor->text().isEmpty()){
        popup->show();
    }
}

void onlineSearchSuggestion::doneCompletion()
{
    timer.stop();
    popup->hide();
    editor->setFocus();
    QTreeWidgetItem *item = popup->currentItem();
    if (item) {
        editor->setText(item->text(0));
        QMetaObject::invokeMethod(editor, "returnPressed");
    }
}

void onlineSearchSuggestion::autoSuggest()
{
    QString str = editor->text();
    QString url = suggestUrl.arg(str);

    if(settingsObj.value("showSearchSuggestion","true").toBool())
    networkManager.get(QNetworkRequest(url));
}

void onlineSearchSuggestion::preventSuggest()
{
    timer.stop();
}

void onlineSearchSuggestion::handleNetworkData(QNetworkReply *networkReply)
{
//    QUrl url = networkReply->url();
    if (networkReply->error() == QNetworkReply::NoError) {
        QVector<QString> choices;

        QByteArray response(networkReply->readAll());
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonArray json_array = jsonResponse.array();
            foreach (const QJsonValue &value, json_array) {
                if(value.isArray()){
                     foreach (const QJsonValue &term, value.toArray()) {
                         if(term.isArray()){
                             foreach (const QJsonValue &val, term.toArray()) {
                                 if(val.isString() && !val.toString().isEmpty() && !checkBlackList(val.toString())){
                                     choices << val.toString();
                                 }
                             }
                         }
                     }
                }
            }
            if(editor->hasFocus()||!editor->text().isEmpty()){
                showCompletion(choices);
            }
    }
    networkReply->deleteLater();
}

bool onlineSearchSuggestion::checkBlackList(QString word){
    bool found = false;
    foreach (const QString blacklisted, blacklist) {
        if(word.contains(blacklisted,Qt::CaseInsensitive)){
            found = true;
        }
    }
    return found;
}
