/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Qt 4.7
import QtMozilla 1.0
import QtQuick 1.0

FocusScope {
    id: root

    anchors.fill: parent
    property alias viewport: webViewport

    signal pageTitleChanged(string title)

    x: 0; y: 0
    width: 100; height: 100

    function load(address) {
        viewport.load(address)
    }

    function focusAddressBar() {
        addressLine.forceActiveFocus()
        addressLine.selectAll()
    }

    Rectangle {
        id: navigationBar
        color: "#efefef"
        height: 30
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Row {
            id: controlsRow
            spacing: 4
            Rectangle {
                id: backButton
                height: navigationBar.height - 2
                width: height
                color: "#efefef"

                Image {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    source: "../icons/backward.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: reloadButton.color
                    opacity: 0.8
                    visible: !viewport.canGoBack
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("going back")
                        viewport.goBack()
                    }
                }
            }
            Rectangle {
                id: forwardButton
                height: navigationBar.height - 2
                width: height
                color: "#efefef"

                Image {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    source: "../icons/forward.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: forwardButton.color
                    opacity: 0.8
                    visible: !viewport.canGoForward
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("going forward")
                        viewport.goForward()
                    }
                }
            }
            Rectangle {
                id: reloadButton
                height: navigationBar.height - 2
                width: height
                color: "#efefef"

                Image {
                    anchors.fill: parent
                    anchors.centerIn: parent
                    source: viewport.canStop ? "../icons/stop.png" : "../icons/refresh.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (viewport.canStop) {
                            console.log("stop loading")
                            viewport.stop()
                        } else {
                            console.log("reloading")
                            viewport.reload()
                        }
                    }
                }
            }
        }
        Rectangle {
            color: "white"
            height: navigationBar.height - 4
            border.width: 1
            anchors {
                left: controlsRow.right
                right: parent.right
                margins: 2
                verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                }
                width: parent.width / 100 * viewport.loadProgress
                color: "blue"
                opacity: 0.3
                visible: viewport.loadProgress != 100
            }

            TextInput {
                id: addressLine
                clip: true
                selectByMouse: true
                font {
                    pointSize: 18
                    family: "Nokia Pure Text"
                }
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    margins: 2
                }

                Keys.onReturnPressed:{
                    console.log("going to: ", addressLine.text)
                    viewport.load(addressLine.text)
                }

                Keys.onPressed: {
                    if (((event.modifiers & Qt.ControlModifier) && event.key == Qt.Key_L) || event.key == Qt.key_F6) {
                        focusAddressBar()
                        event.accepted = true
                    }
                }
            }
        }
        Component.onCompleted: {
            print("QML On Completed>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
        }
        Component.onDestruction: {
            print("QML On Destroyed>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
        }
    }

    QDeclarativeMozView {
        id: webViewport
        visible: true
        focus: true
        anchors {
            top: navigationBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    Keys.onPressed: {
        if (((event.modifiers & Qt.ControlModifier) && event.key == Qt.Key_L) || event.key == Qt.key_F6) {
            console.log("Focus address bar")
            focusAddressBar()
            event.accepted = true
        }
    }
}
