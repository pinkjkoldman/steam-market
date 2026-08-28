#pragma once

#include <QString>

namespace WorkbenchTheme {

inline QString styleSheet() {
  return QStringLiteral(R"QSS(
    QWidget[workbench="true"] {
      background: #0B1726;
      color: #F2F7FB;
      font-family: "Segoe UI Variable", "Microsoft YaHei UI", "Segoe UI";
      font-size: 15px;
    }
    QWidget[workbench="true"] QLabel { color: #F2F7FB; }
    QLabel#pageTitle { color: #F2F7FB; font-size: 22px; font-weight: 600; }
    QLabel#pageSubtitle, QLabel#mutedText { color: #A9B8C7; font-size: 13px; }
    QLabel#sectionTitle { color: #F2F7FB; font-size: 18px; font-weight: 600; }
    QLabel#accountTitle { color: #F2F7FB; font-size: 14px; font-weight: 600; }
    QFrame#surfaceCard, QFrame#modeCard, QFrame#accountCard,
    QFrame#scopeNotice, QFrame#quickInspector {
      background: #101F31;
      border: 1px solid #24364A;
      border-radius: 10px;
    }
    QFrame#modeCard { border-radius: 14px; }
    QFrame#modeCard[recommended="true"] { border-color: #37678E; }
    QLabel#badge { background: #15283C; color: #8CD2FF; border-radius: 6px; padding: 3px 8px; font-size: 13px; font-weight: 600; }
    QLabel#statusLive { color: #35D0A1; }
    QLabel#statusWarning { color: #F3B95F; }
    QLabel#statusError { color: #F17878; }
    QLabel#dataValue { color: #F2F7FB; font-size: 22px; font-weight: 600; }
    QLabel#dataLabel { color: #A9B8C7; font-size: 13px; }
    QLabel#marketMetricCard { background: #101F31; color: #DCEBFA; border: 1px solid #24364A; border-radius: 8px; padding: 7px 10px; font-size: 13px; font-weight: 600; }
    QPushButton { min-height: 34px; padding: 4px 14px; border-radius: 6px; border: 1px solid #37678E; background: #15283C; color: #F2F7FB; }
    QPushButton:hover { border-color: #53B5F4; background: #1A3047; }
    QPushButton:focus { border: 2px solid #8CD2FF; }
    QPushButton:disabled { color: #718397; border-color: #24364A; background: #101F31; }
    QPushButton#primaryButton, QPushButton#batchListingVisibleAction { min-height: 36px; background: #126B9F; border-color: #126B9F; font-weight: 600; }
    QPushButton#primaryButton:hover, QPushButton#batchListingVisibleAction:hover { background: #1A75AB; border-color: #1A75AB; }
    QPushButton#primaryButton:pressed, QPushButton#batchListingVisibleAction:pressed { background: #175F8A; border-color: #175F8A; }
    QPushButton#linkButton { border-color: transparent; background: transparent; color: #53B5F4; padding-left: 4px; padding-right: 4px; }
    QWidget[workbench="true"] QCheckBox { color: #A9B8C7; }
    QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { min-height: 34px; background: #07111F; color: #F2F7FB; border: 1px solid #24364A; border-radius: 6px; padding: 3px 10px; }
    QComboBox { padding-right: 38px; }
    QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 34px; background: #101F31; border-left: 1px solid #37678E; border-top-right-radius: 6px; border-bottom-right-radius: 6px; }
    QComboBox::down-arrow { image: url(:/ui/chevron-down.svg); width: 12px; height: 8px; }
    QComboBox QAbstractItemView { background: #0B1726; color: #F2F7FB; border: 1px solid #37678E; border-radius: 6px; outline: 0; selection-background-color: #1E5277; selection-color: #FFFFFF; padding: 4px; }
    QComboBox QAbstractItemView::item { min-height: 34px; padding: 6px 10px; border-radius: 4px; }
    QComboBox QAbstractItemView::item:selected { background: #1E5277; color: #FFFFFF; }
    QComboBox QAbstractItemView::item:hover { background: #173A56; color: #FFFFFF; }
    QMenu { background: #101F31; color: #F2F7FB; border: 1px solid #37678E; border-radius: 6px; padding: 5px; }
    QMenu::item { min-width: 150px; padding: 8px 14px; border-radius: 4px; }
    QMenu::item:selected { background: #1E5277; color: #FFFFFF; }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 2px solid #8CD2FF; }
    QTableView { background: #0B1726; alternate-background-color: #0F1C2C; color: #F2F7FB; border: 1px solid #24364A; border-radius: 6px; selection-background-color: #1E5277; selection-color: #F2F7FB; }
    QTableView::item { padding: 7px 8px; border-bottom: 1px solid #162A3D; }
    QHeaderView::section { background: #101F31; color: #A9B8C7; border: 0; border-bottom: 1px solid #24364A; padding: 9px 8px; font-weight: 600; }
  )QSS");
}

}  // namespace WorkbenchTheme
