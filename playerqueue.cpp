#include "playerqueue.h"

playerQueue::playerQueue(QWidget *parent)
    : QListWidget(parent)
{
//    setDragEnabled(true);
//    setAcceptDrops(true);
//    setDropIndicatorShown(true);
//    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void playerQueue::addItem(QListWidgetItem *item, bool reusable)
{
    item->setData(ReusableRole, reusable);
    QListWidget::addItem(item);
}


void playerQueue::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem *item = currentItem();
    if (item && item->data(ReusableRole).toBool()) {
        supportedActions = Qt::MoveAction;
    }
    QListWidget::startDrag(supportedActions);
}

void playerQueue::dropEvent(QDropEvent *event)
{
    QListWidget *source = static_cast<QListWidget *>(event->source());
    if (source == this) {
        QListWidgetItem *item = source->currentItem();
        if (item && item->data(ReusableRole).toBool()) {
            QListWidgetItem *item2 = itemAt(QCursor::pos());
            //swap
            setItemWidget(item,itemWidget(item2));
            setItemWidget(item2,itemWidget(item));
        } else {
             qDebug()<<"here2";
            QListWidget::dropEvent(event);
        }
    }
}
