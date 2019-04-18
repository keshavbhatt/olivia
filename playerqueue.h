#ifndef PLAYERQUEUE_H
#define PLAYERQUEUE_H

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDebug>
#include <QListWidgetItem>

class playerQueue : public QListWidget
{
    Q_OBJECT
public:
    using QListWidget::addItem;

    enum PoolItemRole {
        IdentifierRole = Qt::UserRole,
        ReusableRole
    };

    playerQueue(QWidget* parent = nullptr);
    void addItem(QListWidgetItem *item, bool reusable);
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;
};


#endif // PLAYERQUEUE_H
