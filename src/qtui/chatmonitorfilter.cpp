/***************************************************************************
*   Copyright (C) 2005-08 by the Quassel Project                          *
*   devel@quassel-irc.org                                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) version 3.                                           *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "chatmonitorfilter.h"

#include "buffer.h"
#include "client.h"
#include "chatlinemodel.h"
#include "networkmodel.h"

ChatMonitorFilter::ChatMonitorFilter(MessageModel *model, QObject *parent)
  : MessageFilter(model, QList<BufferId>(), parent),
    _initTime(QDateTime::currentDateTime())
{
}

bool ChatMonitorFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  Q_UNUSED(sourceParent)
  Message::Type type = (Message::Type)sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::TypeRole).toInt();
  Message::Flags flags = (Message::Flags)sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::FlagsRole).toInt();
  if(!((type & (Message::Plain | Message::Notice | Message::Action)) || flags & Message::Self))
    return false;

  QDateTime msgTime = sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::TimestampRole).toDateTime();
  return msgTime > _initTime;
}

// override this to inject display of network and channel
QVariant ChatMonitorFilter::data(const QModelIndex &index, int role) const {
  if(index.column() != ChatLineModel::SenderColumn || role != ChatLineModel::DisplayRole)
    return MessageFilter::data(index, role);

  int showFields_ = showFields();
    
  BufferId bufid = data(index, ChatLineModel::BufferIdRole).value<BufferId>();
  if(!bufid.isValid()) {
    qDebug() << "ChatMonitorFilter::data(): chatline belongs to an invalid buffer!";
    return QVariant();
  }
  
  QStringList fields;
  if(showFields_ & NetworkField) {
    fields << Client::networkModel()->networkName(bufid);
  }
  if(showFields_ & BufferField) {
    fields << Client::networkModel()->bufferName(bufid);
  }
  fields << MessageFilter::data(index, role).toString().mid(1);
  return QString("<%1").arg(fields.join(":"));
}

void ChatMonitorFilter::addShowField(int field) {
  QtUiSettings s;
  int fields = s.value(showFieldSettingId(), AllFields).toInt();
  if(fields & field)
    return;

  fields |= field;
  s.setValue(showFieldSettingId(), fields);
  showFieldSettingsChanged();
}

void ChatMonitorFilter::removeShowField(int field) {
  QtUiSettings s;
  int fields = s.value(showFieldSettingId(), AllFields).toInt();
  if(!(fields & field))
    return;

  fields ^= field;
  s.setValue(showFieldSettingId(), fields);
  showFieldSettingsChanged();
}

void ChatMonitorFilter::showFieldSettingsChanged() {
  int rows = rowCount();
  if(rows == 0)
    return;

  emit dataChanged(index(0, ChatLineModel::SenderColumn), index(rows - 1, ChatLineModel::SenderColumn));
}

