#ifndef PDFPREVIEWWIDGET_H
#define PDFPREVIEWWIDGET_H

#include <QWidget>
#include <QSharedPointer>

#include "layout/LayoutEngine.h"

class PdfPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PdfPreviewWidget(QWidget *parent = nullptr);

    void setLayoutEngine(LayoutEnginePtr engine);
    void setCurrentPage(int page);
    int currentPage() const;
    int pageCount() const;
    void setZoom(qreal zoom);
    qreal zoom() const;
    void zoomToFit();

signals:
    void pageChanged(int page);
    void zoomChanged(qreal zoom);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    LayoutEnginePtr m_engine;
    int m_currentPage;
    qreal m_zoom;
    qreal m_highDpiScale;

    void updatePageCount();
};

#endif // PDFPREVIEWWIDGET_H
