#include "appletsfiltering.h"

#include <kglobalsettings.h>
#include <klineedit.h>

#include <plasma/theme.h>

//FilteringTreeView

FilteringTreeView::FilteringTreeView(QGraphicsItem * parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{
    init();
    connect(m_treeView->nativeWidget(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(filterChanged(const QModelIndex &)));
}

FilteringTreeView::~FilteringTreeView()
{
}

void FilteringTreeView::init()
{
    m_treeView = new Plasma::TreeView();
    m_treeView->nativeWidget()->setAttribute(Qt::WA_NoSystemBackground);
    m_treeView->nativeWidget()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->nativeWidget()->setRootIsDecorated(false);
    m_treeView->nativeWidget()->setAttribute(Qt::WA_TranslucentBackground);

    QFont listFont = m_treeView->nativeWidget()->font();
    listFont.setPointSize(KGlobalSettings::smallestReadableFont().pointSize());
    m_treeView->nativeWidget()->setFont(listFont);

    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QPalette plasmaPalette = QPalette();
    plasmaPalette.setColor(QPalette::Base,
                           Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
    plasmaPalette.setColor(QPalette::Text, textColor);

    m_treeView->setPalette(plasmaPalette);
    m_treeView->nativeWidget()->setAutoFillBackground(true);

    QGraphicsLinearLayout *linearLayout = new QGraphicsLinearLayout();
    linearLayout->addItem(m_treeView);
    setLayout(linearLayout);
    m_treeView->nativeWidget()->header()->setVisible(false);
    //m_treeView->nativeWidget()->setPalette((new Plasma::IconWidget())->palette());
}

void FilteringTreeView::setModel(QStandardItemModel *model)
{
    m_model = model;
    m_treeView->setModel(m_model);
}


void FilteringTreeView::filterChanged(const QModelIndex & index)
{
    emit(filterChanged(index.row()));
}

//FilteringTabs

FilteringTabs::FilteringTabs(QGraphicsWidget *parent)
        : Plasma::TabBar(parent)
{
    m_model = 0;
    init();
    connect(this, SIGNAL(currentChanged(int)), this, SIGNAL(filterChanged(int)));
}
FilteringTabs::~FilteringTabs()
{

}

void FilteringTabs::init()
{
    setAttribute(Qt::WA_NoSystemBackground);
    nativeWidget()->setUsesScrollButtons(true);
}

void FilteringTabs::populateList()
{
    QStandardItem *item;
    int indexesCount = m_model->rowCount();

    for(int i = 0; i < indexesCount ; i++){
        item = getItemByProxyIndex(m_model->index(i, 0));
        addTab(item->icon(), item->text());
        if(!item->isEnabled()) {
            nativeWidget()->setTabEnabled(i, false);
        }
    }
}

QStandardItem *FilteringTabs::getItemByProxyIndex(const QModelIndex &index) const
{
    return m_model->itemFromIndex(index);
}

void FilteringTabs::setModel(QStandardItemModel *model)
{
    m_model = model;
    populateList();
}

//FilteringWidget

FilteringWidget::FilteringWidget(QGraphicsItem * parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{
    m_orientation = Qt::Horizontal;
    init();
}

FilteringWidget::FilteringWidget(Qt::Orientation orientation, QGraphicsItem * parent,
                                 Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{
    m_orientation = orientation;
    init();
}

FilteringWidget::~FilteringWidget(){
    delete(m_categoriesTabs);
    delete(m_categoriesTreeView);
}

void FilteringWidget::init()
{
    m_textSearch = new Plasma::LineEdit();
    m_textSearch->nativeWidget()->setClickMessage(/*i18n(*/"Type search"/*)*/);
    m_textSearch->setFocus();
    m_textSearch->setAttribute(Qt::WA_NoSystemBackground);

    m_categoriesTreeView = new FilteringTreeView();
    m_categoriesTabs = new FilteringTabs();

    connect(m_categoriesTreeView, SIGNAL(filterChanged(int)), this, SIGNAL(filterChanged(int)));
    connect(m_categoriesTabs, SIGNAL(filterChanged(int)), this, SIGNAL(filterChanged(int)));

    m_linearLayout = new QGraphicsLinearLayout();
    m_linearLayout->setOrientation(m_orientation);

    m_linearLayout->addItem(m_textSearch);

    if(m_orientation == Qt::Horizontal) {
        m_linearLayout->addItem(m_categoriesTabs);
    } else {
        m_linearLayout->addItem(m_categoriesTreeView);
    }

    setLayout(m_linearLayout);
}

void FilteringWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    Q_UNUSED(event)

    QSizeF contentsSize = m_linearLayout->contentsRect().size();
    m_linearLayout->invalidate();
    m_linearLayout->activate();

    if(m_orientation == Qt::Horizontal) {
        m_textSearch->setMaximumWidth(contentsSize.width()/6);
        m_textSearch->setMinimumWidth(contentsSize.width()/6);
    } else {
        m_textSearch->setMaximumWidth(-1);
        m_textSearch->setMinimumWidth(-1);
    }
}

Plasma::LineEdit *FilteringWidget::textSearch()
{
    return m_textSearch;
}

void FilteringWidget::setModel(QStandardItemModel *model)
{
    m_categoriesTreeView->setModel(model);
    m_categoriesTabs->setModel(model);
}

void FilteringWidget::setListOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    m_linearLayout->setOrientation(orientation);
    m_categoriesTabs->setVisible(false);
    m_categoriesTreeView->setVisible(false);
    m_linearLayout->removeAt(1);
    if(orientation == Qt::Horizontal) {
        m_textSearch->setPreferredWidth(200);
        m_textSearch->setPreferredHeight(-1);
        m_linearLayout->addItem(m_categoriesTabs);
        m_categoriesTabs->setVisible(true);
    } else {
        m_textSearch->setPreferredHeight(30);
        m_textSearch->setPreferredWidth(-1);
        m_linearLayout->addItem(m_categoriesTreeView);
        m_categoriesTreeView->setVisible(true);
    }

    m_linearLayout->invalidate();
}

//FilteringWidgetWithTabs

FilteringWidgetWithTabs::FilteringWidgetWithTabs(QGraphicsItem * parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{
    init();
}

FilteringWidgetWithTabs::~FilteringWidgetWithTabs(){
}

void FilteringWidgetWithTabs::init()
{
    m_textSearch = new Plasma::LineEdit();
    m_textSearch->nativeWidget()->setClickMessage(/*i18n(*/"Type search"/*)*/);
    m_textSearch->setFocus();
    m_textSearch->setAttribute(Qt::WA_NoSystemBackground);

    m_categoriesList = new FilteringTabs();

    QGraphicsLinearLayout *hLayout = new QGraphicsLinearLayout(Qt::Horizontal);

    hLayout->addItem(m_textSearch);
    hLayout->addItem(m_categoriesList);

    hLayout->setContentsMargins(15,15,15,15);
    this->setLayout(hLayout);

}

FilteringTabs *FilteringWidgetWithTabs::categoriesList()
{
    return m_categoriesList;
}

Plasma::LineEdit *FilteringWidgetWithTabs::textSearch()
{
    return m_textSearch;
}
