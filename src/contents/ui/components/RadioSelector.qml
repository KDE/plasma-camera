// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
* @brief A Component that allows sitching between multiple options.
*
* Example:
* @code
* Components.RadioSelector {
*
*   consistentWidth: false
*   actions: [
*       Kirigami.Action {
*           text: i18n("Week")
*           icon.name:  "view-calendar-week"
*       },
*       Kirigami.Action {
*           text: i18n("3 Days")
*           icon.name:  "view-calendar-upcoming-days"
*       },
*       Kirigami.Action {
*           text: i18n("1 Day")
*           icon.name:  "view-calendar-day"
*       }
*   ]
* }
* @endcode
*/
RowLayout {
    id: root

    /**
    * @brief This property holds a list of actions, each holding one of the options.
    */
    property list<Kirigami.Action> actions
    /**
    * @brief This property holds whether all the items should have the same width.
    */
    property bool consistentWidth: false
    /**
    * @brief This property holds which option will be selected by default.
    */
    property int defaultIndex: 0
    /**
    * @brief This property holds the currently selected option.
    */
    readonly property int selectedIndex: marker.selectedIndex
    property string color: Kirigami.Theme.textColor

    Item {
        id: container

        Layout.minimumWidth: consistentWidth ? 0 : switchLayout.implicitWidth
        height: Math.round(Kirigami.Units.gridUnit * 1.5)
        Layout.fillHeight: true
        Layout.fillWidth: consistentWidth

        QQC2.ButtonGroup {
            buttons: switchLayout.children
        }

        RowLayout {
            id: switchLayout

            anchors.fill: parent
            Layout.fillWidth: true

            Repeater {
                id: repeater

                model: actions

                delegate: QQC2.ToolButton {
                    id: button

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    Layout.preferredWidth: consistentWidth ? (root.width / repeater.count) - (switchLayout.spacing / repeater.count - 1) : button.implicitWidth
                    checkable: true
                    text: modelData.text
                    icon.name: modelData.icon.name
                    onClicked: {
                        marker.width = Qt.binding(function() {
                            return width;
                        });
                        marker.x = Qt.binding(function() {
                            return x;
                        });
                        modelData.triggered();
                        marker.selectedIndex = index;
                    }
                    Component.onCompleted: {
                        if (index === defaultIndex) {
                            marker.width = Qt.binding(function() {
                                return width;
                            });
                            marker.x = Qt.binding(function() {
                                return x;
                            });
                            button.checked = true;
                        }
                    }

                    background: Rectangle {
                        anchors.fill: button
                        radius: height / 2
                        color: "transparent"
                        border.color: Kirigami.Theme.disabledTextColor
                        opacity: button.hovered ? 0.3 : 0

                        Behavior on opacity {
                            PropertyAnimation {
                                duration: Kirigami.Units.shortDuration
                                easing.type: Easing.InOutCubic
                            }

                        }

                    }

                    contentItem: RowLayout {
                        Item {
                            Layout.leftMargin: icon.visible ? Kirigami.Units.smallSpacing : Kirigami.Units.largeSpacing
                            Layout.fillWidth: true
                        }

                        Kirigami.Icon {
                            id: icon

                            Layout.topMargin: (container.height - label.height) / 2
                            Layout.bottomMargin: (container.height - label.height) / 2
                            color: button.checked ? Kirigami.Theme.hoverColor : root.color
                            visible: button.icon.name
                            source: button.icon.name
                            implicitHeight: label.height
                            implicitWidth: label.height

                            Behavior on color {
                                PropertyAnimation {
                                    duration: Kirigami.Units.longDuration
                                    easing.type: Easing.InOutCubic
                                }

                            }

                        }

                        Item {
                            Layout.topMargin: (container.height - label.height) / 2
                            Layout.bottomMargin: (container.height - label.height) / 2
                            width: fakeLabel.width
                            height: fakeLabel.height

                            QQC2.Label {
                                id: fakeLabel

                                anchors.centerIn: parent
                                font.bold: true
                                color: Kirigami.Theme.hoverColor
                                opacity: button.checked ? 1 : 0
                                text: button.text

                                Behavior on opacity {
                                    PropertyAnimation {
                                        duration: Kirigami.Units.longDuration
                                        easing.type: Easing.InOutCubic
                                    }

                                }

                            }

                            QQC2.Label {
                                id: label

                                anchors.centerIn: parent
                                color: root.color
                                opacity: button.checked ? 0 : 1
                                text: button.text

                                Behavior on opacity {
                                    PropertyAnimation {
                                        duration: Kirigami.Units.longDuration
                                        easing.type: Easing.InOutCubic
                                    }

                                }

                            }

                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.rightMargin: Kirigami.Units.largeSpacing
                        }

                    }

                }

            }

        }

        Rectangle {
            id: marker

            property int selectedIndex: root.defaultIndex

            y: switchLayout.y
            z: switchLayout.z - 1
            height: container.height
            radius: height / 2
            border.width: 1
            border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.hoverColor, "transparent", 0.4)
            color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.hoverColor, "transparent", 0.9)

            Behavior on x {
                PropertyAnimation {
                    id: x_anim

                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutCubic
                }

            }

            Behavior on width {
                PropertyAnimation {
                    id: width_anim

                    duration: Kirigami.Units.longDuration
                    easing.type: Easing.InOutCubic
                }

            }

        }

    }

}
