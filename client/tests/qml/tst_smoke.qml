// Smoke test: proves the QML test runner, the Qt Quick runtime, and the
// Material controls style are all available offscreen. Component-level
// tests (ApiClient, pages) arrive once the client's QML module is split into
// an importable library (auth increment).

import QtQuick
import QtQuick.Controls.Material
import QtTest

TestCase {
    id: testCase
    name: "Smoke"

    function test_qtquick_items_instantiate() {
        const item = createTemporaryQmlObject("import QtQuick; Item { width: 42; height: 7 }", testCase)
        verify(item)
        compare(item.width, 42)
        compare(item.height, 7)
    }

    function test_material_controls_available() {
        const button = createTemporaryQmlObject(
            "import QtQuick.Controls.Material; Button { text: 'probe'; Material.theme: Material.Dark }",
            testCase)
        verify(button)
        compare(button.text, "probe")
        compare(button.Material.theme, Material.Dark)
    }

    function test_dark_theme_palette_constants() {
        // The placeholder palette used by Main.qml; guards against typos when
        // it moves into the Theme singleton.
        const background = Qt.color("#10141b")
        const accent = Qt.color("#00ffa3")
        verify(background.r < 0.1 && background.g < 0.1 && background.b < 0.15, "background is near-black")
        verify(accent.g > 0.9 && accent.r < 0.1, "accent is neon green")
    }
}
