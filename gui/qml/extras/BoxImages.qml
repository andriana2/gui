import QtQuick 2.15

BoxImagesForm {
    id: boxImages
    state: "bi_draw_robot"
    signal bif_clear_pressed()
    signal bif_ok_pressed()
    signal bif_check_pressed()
    signal bif_edit_pressed()


    ErrorRectangle {
        id: errorPopup
        anchors.centerIn: parent
        errorRectangleTextError.text: "Error: Has puesto el robot en una posicion donde esta prohibido."
    }

    bif_clear.onClicked: {
        boxImages.bif_clear_pressed()
    }
    bif_check.onClicked: {
        boxImages.bif_check_pressed()
    }
    bif_edit.onClicked: {
        boxImages.bif_edit_pressed()
    }
    bif_ok.onClicked: {
        boxImages.bif_ok_pressed()
    }

    states: [
        State {
            name: "bi_draw_robot"
            StateChangeScript {
                script: console.log("estoy en bi_draw_robot")
            }
            PropertyChanges {
                target: bif_check
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_ok
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_clear
                visible: true
                enabled: true
            }
            PropertyChanges {
                target: bif_edit
                visible: true
                enabled: true
            }
            PropertyChanges { target: boxImages; image_source_edit: "../../images/pencil-solid.svg" }
            PropertyChanges { target: boxImages; image_source_clear: "../../images/eraser-solid.svg" }
        },
        State {
            name: "bi_nothing"
            StateChangeScript {
                script: console.log("estoy en bi_nothing")
            }
            PropertyChanges {
                target: bif_check
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_ok
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_clear
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_edit
                visible: false
                enabled: false
            }
        },
        State {
            name: "bi_select_action"
            StateChangeScript {
                script: console.log("estoy en bi_select_action")
            }
            PropertyChanges {
                target: bif_check
                visible: true
                enabled: true
                width: 120
                height: 120
                anchors.verticalCenterOffset: -50
                anchors.horizontalCenterOffset: -90
            }
            PropertyChanges {
                target: bif_ok
                visible: true
                enabled: true
                width: 120
                height: 120
                anchors.verticalCenterOffset: -50
                anchors.horizontalCenterOffset: 40
            }
            PropertyChanges {
                target: bif_clear
                visible: false
                enabled: false
            }
            PropertyChanges {
                target: bif_edit
                visible: false
                enabled: false
            }
            PropertyChanges { target: boxImages; image_source_check: "../../images/map-pin-solid.svg" }
            PropertyChanges { target: boxImages; image_source_ok: "../../images/wheel.svg" }
        },
        State {
            name: "bi_draw_path"
            StateChangeScript {
                script: console.log("estoy en bi_draw_path")
            }
            PropertyChanges {
                target: bif_check
                visible: true
                enabled: true
            }
            PropertyChanges {
                target: bif_ok
                visible: true
                enabled: true
            }
            PropertyChanges {
                target: bif_clear
                visible: true
                enabled: true
            }
            PropertyChanges {
                target: bif_edit
                visible: true
                enabled: true
            }
            PropertyChanges { target: orientationCircle; image_source_check: "../../images/check-solid.svg" }
            PropertyChanges { target: orientationCircle; image_source_ok: "../../images/wheel.svg" }
            PropertyChanges { target: orientationCircle; image_source_clear: "../../images/eraser-solid.svg" }
            PropertyChanges { target: orientationCircle; image_source_edit: "../../images/pencil-solid.svg" }
        }
    ]

}
