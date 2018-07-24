#ifndef STYLE_H
#define STYLE_H

#define OKBTN_STYLE     "QToolButton{font: 10px \"Microsoft YaHei UI Light\";background-color: rgb(84,61,137);border:0px solid white;border-radius:3px;color: white;}"  \
                        "QToolButton:pressed{background-color:rgb(84,61,137);}"\
                        "QToolButton:disabled{background-color:rgb(196,191,214);}"

#define CANCELBTN_STYLE "QToolButton{font: 10px \"Microsoft YaHei UI Light\";background:rgb(235,0,94);color: rgb(255,255,255);border:0px solid rgb(84,116,235);border-radius:3px;}"  \
                        "QToolButton:pressed{background-color:rgb(235,0,94);color: white}"\
                        "QToolButton:disabled{background-color:rgb(196,191,214);}"

#define CLOSEBTN_STYLE  "QToolButton{background-image:url(:/ui/wallet_ui/close_button.png);background-repeat: no-repeat;background-position: center;background-color:transparent;border:none;}"   \
                        "QToolButton:hover{background-color:rgb(208,228,255);"

#define BACKGROUNDWIDGET_STYLE  "#widget {background-image:url(:/ui/wallet_ui/back_dialog.png);}"

#define CONTAINERWIDGET_STYLE   "#containerwidget{background-color:rgb(229,226,240);border-radius:4px;}"  \
                                "QLabel{color:rgb(51,51,51);}"

#define TOOLBUTTON_STYLE_1      "QToolButton{font: 10px \"微软雅黑\";background-color:transparent; border:1px solid rgb(84,61,137);border-radius:9px;color: rgb(84,61,137);}" \
                                "QToolButton:pressed{background-color:rgb(83,61,138);color: white;}"

#define TABLEWIDGET_STYLE_1     "QTableView{background-color:white;border:none;border-radius:15px;font: 12px \"Microsoft YaHei UI Light\";}" \
                                "QHeaderView{border:none;border-bottom:1px solid rgb(240,240,240);border-top-left-radius:15px;border-top-right-radius:15px;background-color:white;color:#C6CAD4;font: 12px \"Microsoft YaHei UI Light\";}" \
                                "QHeaderView:section{height:40px;border:none;background-color:white;}" \
                                "QHeaderView:section:first{border-top-left-radius:15px;}" \
                                "QHeaderView:section:last{border-top-right-radius:15px;}"


#define COMBOBOX_STYLE_BOTTOMBORDER "QComboBox{background-color:transparent;border-top:none;border-left:none;border-right:none;border-bottom:1px solid rgb(192,192,192);color: black;padding-left: 5px;font: 14px \"Microsoft YaHei UI Light\";}" \
                                    "QComboBox::drop-down {background:transparent;border:none;}" \
                                    "QComboBox::down-arrow {image: url(:/ui/wallet_ui/downArrow.png);}" \
                                    "QComboBox::down-arrow:on {image: url(:/ui/wallet_ui/downArrow_on.png);}" \
                                    "QComboBox QAbstractItemView {outline: 0px;}"

#define FUNCTIONBAR_PUSHBUTTON_STYLE    "QPushButton{color:rgb(110,94,144);text-align:left;background:transparent;border:none;font-size:12px;font-family:\"Microsoft YaHei UI Light\";}" \
                                        "QPushButton:checked{color:rgb(233,232,233);border-bottom:1px solid rgb(229,226,240);}"

#define BIG_BUTTON      "QToolButton{font: 11px \"Microsoft YaHei UI Light\";background-color:rgb(84,116,235); border:none;border-radius:15px;color: rgb(255, 255, 255);}" \
                                "QToolButton:pressed{background-color:rgb(70,95,191);}"

#define OKBTN_STYLE_HOVER     "QToolButton{font: 14px \"Microsoft YaHei UI Light\";background-color: rgb(84,116,235);border:0px solid white;border-radius:15px;color: white;}"  \
                        "QToolButton:hover{background-color:rgb(0,210,255);}"   \
                        "QToolButton:disabled{background-color:rgb(235,235,235);}"

#define CANCELBTN_STYLE_HOVER "QToolButton{background:rgb(229,229,229);color: white;border:0px solid white;border-radius:15px;}"  \
                        "QToolButton:hover{background-color:rgb(0,210,255);}"


#define MENU_STYLE      "QMenu {border-radius:10px;background-color:rgba(238,241,253,235);border: 0px solid red;}"\
                        "QMenu::item {border-radius:10px;border: 0px solid green;background-color:transparent;padding:5px 10px;}"\
                        "QMenu::item:selected {background-color:rgb(130,157,255);}"\
                        "QMenu::separator {height: 2px;background-color: #FCFCFC;}"\
                        "QMenu::right-arrow {padding:0px 10px;image:url(:/wallet_ui/right.png);}"


#endif // STYLE_H
