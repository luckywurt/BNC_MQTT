QT += printer widgets
QT += svg

TEMPLATE = subdirs

CONFIG += c++11
CONFIG += ordered

SUBDIRS = newmat   \
          qwt      \
          qwtpolar \
          src
