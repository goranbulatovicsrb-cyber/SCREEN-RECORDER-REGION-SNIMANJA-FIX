#include "AnnotationWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QLabel>
#include <QColorDialog>
#include <QInputDialog>
#include <QScrollArea>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPainterPath>
#include <QtMath>
#include <QCursor>
#include <QMessageBox>
#include <QStyle>
#include <QShortcut>

// ─── AnnotCanvas ─────────────────────────────────────────────────────────────

AnnotCanvas::AnnotCanvas(const QPixmap &px, QWidget *parent)
    : QWidget(parent), m_orig(px)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

// Map widget point to image point
QPoint AnnotCanvas::toImg(QPoint wp) const {
    QRect ir = imageRect();
    if (ir.isEmpty()) return wp;
    double sx = (double)m_orig.width()  / ir.width();
    double sy = (double)m_orig.height() / ir.height();
    return QPoint((wp.x() - ir.left()) * sx, (wp.y() - ir.top()) * sy);
}

QPoint AnnotCanvas::toWid(QPoint ip) const {
    QRect ir = imageRect();
    if (ir.isEmpty()) return ip;
    double sx = (double)ir.width()  / m_orig.width();
    double sy = (double)ir.height() / m_orig.height();
    return QPoint(ir.left() + ip.x() * sx, ir.top() + ip.y() * sy);
}

// Image centered in canvas with aspect ratio
QRect AnnotCanvas::imageRect() const {
    if (m_orig.isNull()) return rect();
    QSize s = m_orig.size().scaled(size(), Qt::KeepAspectRatio);
    int x = (width()  - s.width())  / 2;
    int y = (height() - s.height()) / 2;
    return QRect(x, y, s.width(), s.height());
}

void AnnotCanvas::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); update(); }

void AnnotCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Dark background
    p.fillRect(rect(), QColor(0x0d, 0x0d, 0x1e));

    // Draw image
    QRect ir = imageRect();
    if (!m_orig.isNull()) {
        p.drawPixmap(ir, m_orig);
        // Thin border
        p.setPen(QPen(QColor(0, 174, 255, 60), 1));
        p.drawRect(ir);
    }

    // Draw committed annotations (scaled to widget coords)
    for (const auto &a : m_annots) drawAnnot(p, a);

    // Draw current in-progress annotation
    if (m_drawing) drawAnnot(p, m_cur);
}

void AnnotCanvas::drawAnnot(QPainter &p, const Annot &a, bool) const {
    QPoint w1 = toWid(a.p1), w2 = toWid(a.p2);
    QRect  wr  = QRect(w1, w2).normalized();

    switch (a.tool) {
    case AnnotTool::Arrow:
        drawArrow(p, w1, w2, a.width, a.color);
        break;

    case AnnotTool::Rect:
        p.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (a.filled) p.setBrush(QColor(a.color.red(), a.color.green(), a.color.blue(), 70));
        else           p.setBrush(Qt::NoBrush);
        p.drawRect(wr);
        break;

    case AnnotTool::Ellipse:
        p.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (a.filled) p.setBrush(QColor(a.color.red(), a.color.green(), a.color.blue(), 70));
        else           p.setBrush(Qt::NoBrush);
        p.drawEllipse(wr);
        break;

    case AnnotTool::Line:
        p.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(w1, w2);
        break;

    case AnnotTool::Freehand: {
        if (a.pts.size() < 2) break;
        QPainterPath path;
        path.moveTo(toWid(a.pts[0]));
        for (int i = 1; i < a.pts.size(); ++i) path.lineTo(toWid(a.pts[i]));
        p.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
        break;
    }

    case AnnotTool::Text:
        if (!a.text.isEmpty()) {
            QFont f = a.font; f.setPixelSize(qMax(12, (int)(18 * (double)width()/qMax(1,m_orig.width()))));
            p.setFont(f);
            // Shadow
            p.setPen(QColor(0,0,0,160));
            p.drawText(w1 + QPoint(2,2), a.text);
            p.setPen(a.color);
            p.drawText(w1, a.text);
        }
        break;

    case AnnotTool::Highlight: {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(a.color.red(), a.color.green(), a.color.blue(), 100));
        p.drawRect(wr);
        break;
    }

    case AnnotTool::Blur: {
        // Visual indicator (actual blur applied on flatten)
        p.setPen(QPen(a.color, 2, Qt::DashLine));
        p.setBrush(QColor(0,0,0,60));
        p.drawRect(wr);
        QFont f; f.setPixelSize(11);
        p.setFont(f); p.setPen(Qt::white);
        p.drawText(wr, Qt::AlignCenter, "BLUR");
        break;
    }

    case AnnotTool::StepNum: {
        int r = qMax(12, a.width * 5);
        p.setBrush(a.color); p.setPen(Qt::NoPen);
        p.drawEllipse(w1, r, r);
        QFont f; f.setPixelSize(r); f.setBold(true);
        p.setFont(f); p.setPen(Qt::white);
        p.drawText(QRect(w1.x()-r, w1.y()-r, r*2, r*2),
                   Qt::AlignCenter, QString::number(a.step));
        break;
    }
    }
}

void AnnotCanvas::drawArrow(QPainter &p, QPoint from, QPoint to,
                              int w, QColor c) const {
    if (from == to) return;
    p.setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(from, to);

    double angle = std::atan2(-(to.y()-from.y()), to.x()-from.x());
    double len   = 14 + w * 2.5;
    double ang   = M_PI / 7.0;

    QPointF p1(to.x() - len*std::cos(angle - ang), to.y() + len*std::sin(angle - ang));
    QPointF p2(to.x() - len*std::cos(angle + ang), to.y() + len*std::sin(angle + ang));

    p.setBrush(c); p.setPen(Qt::NoPen);
    QPolygonF head; head << QPointF(to) << p1 << p2;
    p.drawPolygon(head);
}

void AnnotCanvas::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;
    if (!imageRect().contains(e->pos())) return;

    if (m_tool == AnnotTool::Text) {
        bool ok;
        QString txt = QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "", &ok);
        if (!ok || txt.isEmpty()) return;
        m_undo.push(m_annots); m_redo.clear();
        Annot a; a.tool=AnnotTool::Text; a.text=txt;
        a.p1=toImg(e->pos()); a.p2=a.p1;
        a.color=m_color; a.width=m_width;
        QFont f; f.setPixelSize(22); f.setBold(true); a.font=f;
        m_annots.append(a);
        update(); emit changed(); return;
    }

    m_drawing = true;
    m_undo.push(m_annots); m_redo.clear();

    m_cur = Annot();
    m_cur.tool   = m_tool;
    m_cur.p1     = toImg(e->pos());
    m_cur.p2     = m_cur.p1;
    m_cur.color  = m_color;
    m_cur.width  = m_width;
    m_cur.filled = m_filled;
    m_cur.step   = m_step;
    QFont f; f.setPixelSize(22); f.setBold(true); m_cur.font = f;

    if (m_tool == AnnotTool::Freehand || m_tool == AnnotTool::Highlight)
        m_cur.pts.append(m_cur.p1);
}

void AnnotCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (!m_drawing) return;
    m_cur.p2 = toImg(e->pos());
    if (m_tool == AnnotTool::Freehand || m_tool == AnnotTool::Highlight)
        m_cur.pts.append(m_cur.p2);
    update();
}

void AnnotCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (!m_drawing || e->button() != Qt::LeftButton) return;
    m_drawing = false;
    m_cur.p2 = toImg(e->pos());

    if (m_tool == AnnotTool::StepNum) m_step++;

    m_annots.append(m_cur);
    update(); emit changed();
}

void AnnotCanvas::undo() {
    if (!m_undo.isEmpty()) {
        m_redo.push(m_annots);
        m_annots = m_undo.pop();
        update();
    }
}

void AnnotCanvas::redo() {
    if (!m_redo.isEmpty()) {
        m_undo.push(m_annots);
        m_annots = m_redo.pop();
        update();
    }
}

void AnnotCanvas::clearAll() {
    m_undo.push(m_annots);
    m_annots.clear();
    m_step = 1;
    update();
}

QPixmap AnnotCanvas::flatten() const {
    // Draw image + annotations at original resolution
    QPixmap result(m_orig.size());
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0, 0, m_orig);

    // Scale factor: we need to draw annotations in original image coords
    // Since annotations are stored in image coords, draw them directly
    double scaleX = (double)m_orig.width()  / width();
    double scaleY = (double)m_orig.height() / height();
    Q_UNUSED(scaleX); Q_UNUSED(scaleY);

    // Save and restore painter for each annot
    for (const auto &a : m_annots) {
        // Draw in image coords (stored coords ARE image coords)
        QPoint w1 = a.p1, w2 = a.p2;
        QRect  wr = QRect(w1, w2).normalized();

        switch (a.tool) {
        case AnnotTool::Arrow:  drawArrow(p, w1, w2, a.width, a.color); break;
        case AnnotTool::Rect:
            p.setPen(QPen(a.color, a.width));
            if (a.filled) p.setBrush(QColor(a.color.red(),a.color.green(),a.color.blue(),70));
            else p.setBrush(Qt::NoBrush);
            p.drawRect(wr); break;
        case AnnotTool::Ellipse:
            p.setPen(QPen(a.color, a.width));
            if (a.filled) p.setBrush(QColor(a.color.red(),a.color.green(),a.color.blue(),70));
            else p.setBrush(Qt::NoBrush);
            p.drawEllipse(wr); break;
        case AnnotTool::Line:
            p.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(w1, w2); break;
        case AnnotTool::Freehand: {
            if (a.pts.size()<2) break;
            QPainterPath path; path.moveTo(a.pts[0]);
            for (int i=1;i<a.pts.size();++i) path.lineTo(a.pts[i]);
            p.setPen(QPen(a.color,a.width,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            p.setBrush(Qt::NoBrush); p.drawPath(path); break; }
        case AnnotTool::Text: {
            p.setFont(a.font); p.setPen(QColor(0,0,0,160));
            p.drawText(w1+QPoint(1,1), a.text);
            p.setPen(a.color); p.drawText(w1, a.text); break; }
        case AnnotTool::Highlight:
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(a.color.red(),a.color.green(),a.color.blue(),100));
            p.drawRect(wr); break;
        case AnnotTool::Blur: {
            // Apply actual blur to the region
            QRect blurRect = wr.intersected(result.rect());
            if (!blurRect.isEmpty()) {
                QPixmap region = result.copy(blurRect);
                QImage  blurred= region.toImage();
                // Box blur 8x
                for (int pass=0; pass<8; ++pass) {
                    QImage tmp = blurred;
                    for (int y=1;y<blurred.height()-1;++y) {
                        QRgb *row = reinterpret_cast<QRgb*>(blurred.scanLine(y));
                        const QRgb *prev=reinterpret_cast<const QRgb*>(tmp.constScanLine(y-1));
                        const QRgb *next=reinterpret_cast<const QRgb*>(tmp.constScanLine(y+1));
                        for (int x=1;x<blurred.width()-1;++x) {
                            row[x]=qRgb(
                                (qRed(prev[x])+qRed(row[x-1])+qRed(row[x])+qRed(row[x+1])+qRed(next[x]))/5,
                                (qGreen(prev[x])+qGreen(row[x-1])+qGreen(row[x])+qGreen(row[x+1])+qGreen(next[x]))/5,
                                (qBlue(prev[x])+qBlue(row[x-1])+qBlue(row[x])+qBlue(row[x+1])+qBlue(next[x]))/5);
                        }
                    }
                }
                p.drawImage(blurRect, blurred);
            }
            break; }
        case AnnotTool::StepNum: {
            int r=18; p.setBrush(a.color); p.setPen(Qt::NoPen);
            p.drawEllipse(w1,r,r);
            QFont f; f.setPixelSize(14); f.setBold(true); p.setFont(f);
            p.setPen(Qt::white);
            p.drawText(QRect(w1.x()-r,w1.y()-r,r*2,r*2),Qt::AlignCenter,QString::number(a.step));
            break; }
        }
    }
    return result;
}

// ─── AnnotationWidget ─────────────────────────────────────────────────────────

AnnotationWidget::AnnotationWidget(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("ScreenMaster Pro — Annotate");
    setMinimumSize(1000, 700);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setStyleSheet(R"(
QWidget { background:#0d0d1e; color:#e0e0f0;
          font-family:"Segoe UI",Arial,sans-serif; font-size:13px; }
QPushButton { background:#1e1e3a; border:1px solid #303055;
              border-radius:6px; padding:6px 14px; color:#ccccee; }
QPushButton:hover { background:#2a2a50; border-color:#00aeff; color:#fff; }
QPushButton#active { background:#00aeff; color:#fff; font-weight:bold;
                     border:none; }
QPushButton#done { background:#0099dd; color:white; font-weight:bold;
                   border:none; border-radius:6px; padding:8px 20px; }
QPushButton#done:hover { background:#00aaee; }
QPushButton#danger { background:rgba(180,40,40,0.3); color:#ee6666; }
QSpinBox { background:#1e1e3a; border:1px solid #303055; border-radius:5px;
           color:#d0d0f0; padding:3px 6px; }
QLabel { color:#aaa; }
)");

    m_canvas = new AnnotCanvas(pixmap, this);
    buildUI();
}

void AnnotationWidget::buildUI() {
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    // Toolbar
    QWidget *tb = new QWidget();
    tb->setFixedHeight(52);
    tb->setStyleSheet("background:#0a0a18; border-bottom:2px solid #00aeff;");
    buildToolbar(tb);
    main->addWidget(tb);

    // Canvas
    main->addWidget(m_canvas, 1);

    // Palette bar
    QWidget *pb = new QWidget();
    pb->setFixedHeight(54);
    pb->setStyleSheet("background:#0a0a18; border-top:1px solid #202040;");
    buildPalette(pb);
    main->addWidget(pb);

    // Shortcuts
    auto addSc = [this](const QKeySequence &k, auto fn) {
        auto *sc = new QShortcut(k, this);
        connect(sc, &QShortcut::activated, this, fn);
    };
    addSc(QKeySequence("Ctrl+Z"), [this](){ m_canvas->undo(); });
    addSc(QKeySequence("Ctrl+Y"), [this](){ m_canvas->redo(); });
    addSc(QKeySequence("Escape"), [this](){ close(); });
}

void AnnotationWidget::buildToolbar(QWidget *tb) {
    QHBoxLayout *lay = new QHBoxLayout(tb);
    lay->setContentsMargins(14,6,14,6);
    lay->setSpacing(6);

    QLabel *title = new QLabel("✏️  Annotation Mode");
    title->setStyleSheet("font-size:14px; font-weight:bold; color:#00aeff; background:transparent;");
    lay->addWidget(title);
    lay->addSpacing(20);

    // Tool buttons
    struct TB { QString icon; QString tip; AnnotTool tool; };
    QList<TB> tools = {
        {"↗",  "Arrow (mark direction)", AnnotTool::Arrow},
        {"▭",  "Rectangle",              AnnotTool::Rect},
        {"◯",  "Ellipse",                AnnotTool::Ellipse},
        {"╱",  "Line",                   AnnotTool::Line},
        {"✏",  "Freehand Draw",          AnnotTool::Freehand},
        {"T",  "Add Text",               AnnotTool::Text},
        {"▌",  "Highlight",              AnnotTool::Highlight},
        {"⬜",  "Blur / Redact",          AnnotTool::Blur},
        {"①",  "Step Number",            AnnotTool::StepNum},
    };

    QButtonGroup *grp = new QButtonGroup(this);
    grp->setExclusive(true);
    bool first = true;
    for (const auto &tb2 : tools) {
        QPushButton *btn = new QPushButton(tb2.icon);
        btn->setToolTip(tb2.tip);
        btn->setFixedSize(38,38);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        if (first) { btn->setChecked(true); btn->setObjectName("active"); first=false; }
        grp->addButton(btn);
        m_toolBtns.append(btn);
        lay->addWidget(btn);

        AnnotTool t = tb2.tool;
        connect(btn, &QPushButton::clicked, this, [this, t, btn](){
            setActiveTool(t, btn);
        });
    }

    lay->addSpacing(20);

    // Width
    QLabel *wl = new QLabel("Width:"); wl->setStyleSheet("color:#888; background:transparent;");
    lay->addWidget(wl);
    QSpinBox *ws = new QSpinBox();
    ws->setRange(1,30); ws->setValue(3); ws->setFixedWidth(65);
    connect(ws, QOverload<int>::of(&QSpinBox::valueChanged),
            m_canvas, &AnnotCanvas::setWidth);
    lay->addWidget(ws);

    lay->addStretch();

    // Actions
    auto makeBtn = [&](const QString &lbl, const QString &tip, const QString &obj="") {
        QPushButton *b = new QPushButton(lbl);
        b->setToolTip(tip);
        b->setFixedHeight(36);
        b->setCursor(Qt::PointingHandCursor);
        if (!obj.isEmpty()) b->setObjectName(obj);
        return b;
    };

    QPushButton *undoBtn  = makeBtn("↩ Undo",  "Undo (Ctrl+Z)");
    QPushButton *redoBtn  = makeBtn("↪ Redo",  "Redo (Ctrl+Y)");
    QPushButton *clearBtn = makeBtn("🗑 Clear", "Clear all");
    QPushButton *copyBtn  = makeBtn("📋 Copy",  "Copy to clipboard");
    QPushButton *saveBtn  = makeBtn("💾 Save",  "Save as file");
    QPushButton *doneBtn  = makeBtn("✔ Done",   "Apply annotations", "done");

    connect(undoBtn,  &QPushButton::clicked, m_canvas, &AnnotCanvas::undo);
    connect(redoBtn,  &QPushButton::clicked, m_canvas, &AnnotCanvas::redo);
    connect(clearBtn, &QPushButton::clicked, m_canvas, &AnnotCanvas::clearAll);
    connect(copyBtn,  &QPushButton::clicked, this, [this](){
        QApplication::clipboard()->setPixmap(m_canvas->flatten());
    });
    connect(saveBtn, &QPushButton::clicked, this, [this](){
        QString p = QFileDialog::getSaveFileName(this,"Save","annotated.png",
            "PNG (*.png);;JPEG (*.jpg)");
        if (!p.isEmpty()) m_canvas->flatten().save(p);
    });
    connect(doneBtn, &QPushButton::clicked, this, [this](){
        emit annotationComplete(m_canvas->flatten());
        close();
    });

    for (auto *b : {undoBtn,redoBtn,clearBtn,copyBtn,saveBtn,doneBtn})
        lay->addWidget(b);
}

void AnnotationWidget::buildPalette(QWidget *pb) {
    QHBoxLayout *lay = new QHBoxLayout(pb);
    lay->setContentsMargins(16,8,16,8);
    lay->setSpacing(8);

    QLabel *cl = new QLabel("Color:");
    cl->setStyleSheet("color:#888; background:transparent;");
    lay->addWidget(cl);

    // Preset colors — modern flat palette
    QList<QColor> presets = {
        QColor(255, 60,  60),   // Red
        QColor(255, 140,  0),   // Orange
        QColor(255, 230,  0),   // Yellow
        QColor( 60, 220, 80),   // Green
        QColor(  0, 180, 255),  // Blue
        QColor(160,  80, 255),  // Purple
        QColor(255, 100, 180),  // Pink
        QColor(255, 255, 255),  // White
        QColor( 20,  20,  20),  // Black
    };

    for (const auto &c : presets) {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(30, 30);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(c.name().toUpper());
        btn->setStyleSheet(QString(
            "QPushButton { background:%1; border:2px solid #404060; border-radius:15px; }"
            "QPushButton:hover { border:2px solid #ffffff; border-radius:15px; }"
        ).arg(c.name()));
        connect(btn, &QPushButton::clicked, this, [this, c](){
            m_activeColor = c;
            m_canvas->setColor(c);
        });
        lay->addWidget(btn);
    }

    // Custom color picker
    QPushButton *customBtn = new QPushButton("+ Custom");
    customBtn->setFixedHeight(30);
    customBtn->setCursor(Qt::PointingHandCursor);
    connect(customBtn, &QPushButton::clicked, this, [this](){
        QColor c = QColorDialog::getColor(m_activeColor, this, "Pick Color",
                                          QColorDialog::ShowAlphaChannel);
        if (c.isValid()) { m_activeColor = c; m_canvas->setColor(c); }
    });
    lay->addWidget(customBtn);

    lay->addSpacing(24);

    // Fill toggle
    QPushButton *fillBtn = new QPushButton("■ Fill");
    fillBtn->setCheckable(true);
    fillBtn->setFixedHeight(30);
    fillBtn->setCursor(Qt::PointingHandCursor);
    connect(fillBtn, &QPushButton::toggled, m_canvas, &AnnotCanvas::setFilled);
    lay->addWidget(fillBtn);

    lay->addStretch();

    // Keyboard shortcuts hint
    QLabel *hint = new QLabel("Ctrl+Z Undo  |  Ctrl+Y Redo  |  ESC Close");
    hint->setStyleSheet("color:#444; font-size:11px; background:transparent;");
    lay->addWidget(hint);
}

void AnnotationWidget::setActiveTool(AnnotTool t, QPushButton *btn) {
    m_canvas->setTool(t);
    for (auto *b : m_toolBtns) {
        b->setChecked(false);
        b->setObjectName("");
        b->setStyleSheet("");
    }
    if (btn) {
        btn->setChecked(true);
        btn->setObjectName("active");
        btn->setStyleSheet("background:#00aeff; color:white; font-weight:bold; border:none;");
    }
}
