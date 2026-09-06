// Regression tests for the Project-explorer workspace model (workspace_model.h),
// specifically the empty-state row count that drives the EmptyHintTreeView
// "No types yet" overlay in the workspace dock.
#include "workspace_model.h"
#include <QtTest/QtTest>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>

using namespace rcx;

class TestWorkspace : public QObject {
    Q_OBJECT
private slots:
    // An empty project (no tabs) must yield ZERO rows so the tree's empty-state
    // overlay can fire. Regression: buildProjectExplorer used to append an
    // UNCONDITIONAL "ALL TYPES" section header, leaving the model permanently at
    // rowCount >= 1 — the overlay's `rowCount > 0 -> return` guard then never
    // painted the placeholder on a genuinely empty project.
    void testEmptyProjectHasNoRows() {
        QStandardItemModel model;
        buildProjectExplorer(&model, {}, {});
        QCOMPARE(model.rowCount(), 0);
    }

    // A tab whose tree holds no top-level Struct types is empty for the explorer
    // too (only Struct nodes are listed), so still zero rows.
    void testNonStructTabHasNoRows() {
        NodeTree tree;
        Node n; n.kind = NodeKind::Hex64; n.parentId = 0; tree.addNode(n);
        QVector<TabInfo> tabs{ TabInfo{ &tree, QStringLiteral("T"), nullptr } };
        QStandardItemModel model;
        buildProjectExplorer(&model, tabs, {});
        QCOMPARE(model.rowCount(), 0);
    }

    // One struct type → "ALL TYPES" header row + 1 type row (header still emits
    // when there's content under it).
    void testStructTabHasHeaderAndRow() {
        NodeTree tree;
        Node s; s.kind = NodeKind::Struct;
        s.structTypeName = QStringLiteral("MyType"); s.parentId = 0;
        tree.addNode(s);
        QVector<TabInfo> tabs{ TabInfo{ &tree, QStringLiteral("T"), nullptr } };
        QStandardItemModel model;
        buildProjectExplorer(&model, tabs, {});
        QCOMPARE(model.rowCount(), 2);   // ALL TYPES header + the type
        QVERIFY(!model.item(0)->data(RoleSectionHeader).toString().isEmpty());
        QVERIFY(model.item(1)->data(RoleSectionHeader).toString().isEmpty());
    }

    // Section rows follow the ONE shared SectionHeader spec (8 pt regular
    // uppercase, height fm.height() + 8). The delegate used to carry its own
    // 0.67x font and a fm.height() + 16 row, so the Project tree's dividers
    // never matched the scanner's.
    void testSectionRowUsesTheSharedHeaderSpec() {
        WorkspaceDelegate d;
        QStandardItemModel model;
        NodeTree tree;
        Node s; s.kind = NodeKind::Struct;
        s.structTypeName = QStringLiteral("MyType"); s.parentId = 0;
        tree.addNode(s);
        QVector<TabInfo> tabs{ TabInfo{ &tree, QStringLiteral("T"), nullptr } };
        buildProjectExplorer(&model, tabs, {});

        QStyleOptionViewItem opt;
        opt.font = QFont(QStringLiteral("JetBrains Mono"), 10);
        opt.fontMetrics = QFontMetrics(opt.font);
        const int h = d.sizeHint(opt, model.index(0, 0)).height();
        // Measured with the metrics of the font the row is PAINTED with (the
        // derived 8 pt), not the 10 pt base — that mismatch is what made the
        // Project tree's bands taller than the scanner's.
        QCOMPARE(h, sectionHeaderHeight(
                        QFontMetrics(sectionHeaderFont(opt.font))));
        QVERIFY(h != sectionHeaderHeight(opt.fontMetrics));
        // ... and a data row is NOT that height (the two must stay distinct).
        QVERIFY(d.sizeHint(opt, model.index(1, 0)).height() != h);
    }

    // Selection accent = indHoverSpan, the one "current / selected" hue.
    // borderFocused is the FOCUS RING colour (window frames, input focus) and
    // meant the selected row read as a second, competing accent.
    void testSelectionAccentIsTheSharedAccent() {
        Theme t = ThemeManager::instance().current();
        t.indHoverSpan  = QColor(11, 22, 33);
        t.borderFocused = QColor(44, 55, 66);
        WorkspaceDelegate d;
        d.setThemeColors(t);
        QCOMPARE(d.accentColor(), t.indHoverSpan);
    }
};

QTEST_MAIN(TestWorkspace)
#include "test_workspace.moc"
