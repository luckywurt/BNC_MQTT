// Part of BNC, a utility for retrieving decoding and
// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2007
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Czech Technical University Prague, Department of Geodesy
// http://www.fsv.cvut.cz
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      bncHlpDlg
 *
 * Purpose:    Displays the help
 *
 * Author:     L. Mervart
 *
 * Created:    24-Sep-2006
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <iostream>
#include "bnchlpdlg.h"
#include "bnchtml.h"

// Constructor
////////////////////////////////////////////////////////////////////////////
bncHlpDlg::bncHlpDlg(QWidget* parent, const QUrl& url) :
                    QDialog(parent) {

  const int ww = QFontMetrics(font()).horizontalAdvance('w');

  _tb = new bncHtml;
  setWindowTitle("Help Contents");
  _tb->setSource(url);
  _tb->setReadOnly(true);
  connect(_tb, SIGNAL(backwardAvailable(bool)), this, SLOT(backwardAvailable(bool)));
  connect(_tb, SIGNAL(forwardAvailable(bool)),  this, SLOT(forwardAvailable(bool)));

  QVBoxLayout* dlgLayout = new QVBoxLayout;
  dlgLayout->addWidget(_tb);

  QHBoxLayout* butLayout = new QHBoxLayout;

  _backButton = new QPushButton("Backward");
  _backButton->setMaximumWidth(10*ww);
  _backButton->setEnabled(false);
  connect(_backButton, SIGNAL(clicked()), _tb, SLOT(backward()));
  butLayout->addWidget(_backButton);

  _forwButton = new QPushButton("Forward");
  _forwButton->setMaximumWidth(10*ww);
  _forwButton->setEnabled(false);
  connect(_forwButton, SIGNAL(clicked()), _tb, SLOT(forward()));
  butLayout->addWidget(_forwButton);

  _closeButton = new QPushButton("Close");
  _closeButton->setMaximumWidth(10*ww);
  connect(_closeButton, SIGNAL(clicked()), this, SLOT(close()));
  butLayout->addWidget(_closeButton);

  _label = new QLabel("Find &what:"); // 2.
  _what  = new QLineEdit();
  _label->setBuddy(_what); // 3.
  _cbCase = new QCheckBox("Match &case");
  _cbBack = new QCheckBox("Search &backward");
  _findButton = new QPushButton("&Find"); // 4.
  _findButton->setDefault(true);
  _findButton->setEnabled(false);
  connect(_what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
  connect(_findButton, SIGNAL(clicked()), this, SLOT(findClicked()));

  butLayout->addWidget(_label);
  butLayout->addWidget(_what);
  butLayout->addWidget(_findButton);
  butLayout->addWidget(_cbCase);
  butLayout->addWidget(_cbBack);

  connect(this, SIGNAL(findNext(const QString &, Qt::CaseSensitivity)),
          this, SLOT(slotFindNext(const QString &, Qt::CaseSensitivity)));

  connect(this, SIGNAL(findPrev(const QString &, Qt::CaseSensitivity)),
          this, SLOT(slotFindPrev(const QString &, Qt::CaseSensitivity)));

  _hlCursor =  QTextCursor(_tb->document());

  dlgLayout->addLayout(butLayout);

  setLayout(dlgLayout);
  resize(110*ww, 100*ww);
  show();
}

// Destructor
////////////////////////////////////////////////////////////////////////////
bncHlpDlg::~bncHlpDlg() {
  delete _tb;
  delete _backButton;
  delete _forwButton;
  delete _closeButton;
  delete _findButton;
  delete _label;
  delete _what;
  delete _cbCase;
  delete _cbBack;
}

// Slots
////////////////////////////////////////////////////////////////////////////
void bncHlpDlg::backwardAvailable(bool avail) {
  _backButton->setEnabled(avail);
}

void bncHlpDlg::forwardAvailable(bool avail) {
  _forwButton->setEnabled(avail);
}

void bncHlpDlg::findClicked() {

  QString text = _what->text();

  Qt::CaseSensitivity cs = _cbCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

  if(_cbBack->isChecked()) {
      emit findPrev(text, cs);
  }
  else {
      emit findNext(text, cs);
  }
}

void bncHlpDlg::enableBtnFind(const QString& text) {
  _findButton->setEnabled(!text.isEmpty());
}

void bncHlpDlg::slotFindNext(const QString& str, Qt::CaseSensitivity cs) {
  QString searchString = str;
  QTextDocument::FindFlags flags;
  flags |= QTextDocument::FindWholeWords;
  if ( cs == Qt::CaseSensitive) {
      flags |= QTextDocument::FindCaseSensitively;
  }

  _hlCursor = (_tb->document()->find(searchString, _hlCursor, flags));
  if (!_hlCursor.isNull()) {
    _hlCursor.movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
    _tb->setTextCursor(_hlCursor);
  }
  else {
    _hlCursor.movePosition(QTextCursor::Start);
    _tb->setTextCursor(_hlCursor);
  }

}


void bncHlpDlg::slotFindPrev(const QString& str, Qt::CaseSensitivity cs) {
  QString searchString = str;
  QTextDocument::FindFlags flags;
  flags |= QTextDocument::FindWholeWords;
  flags |= QTextDocument::FindBackward;
  if ( cs == Qt::CaseSensitive) {
      flags |= QTextDocument::FindCaseSensitively;
  }

  _hlCursor = (_tb->document()->find(searchString, _hlCursor, flags));
  if (!_hlCursor.isNull()) {
    _hlCursor.movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
    _tb->setTextCursor(_hlCursor);
  }
  else {
    _hlCursor.movePosition(QTextCursor::End);
    _tb->setTextCursor(_hlCursor);
  }

}





