#include "TreeView.h"
#include "TreeViewDelegate.h"
#include "TreeViewHeader.h"
#include <QStandardItemModel>

TreeView::TreeView(QWidget* parent):
QTreeView(parent),
m_treeViewDelegate(nullptr),
m_treeViewHeader(nullptr)
{
	m_treeViewDelegate = new TreeViewDelegate;
	m_treeViewHeader = new TreeViewHeader(this);
	m_treeViewModel = new TreeViewModel(this);
	init();
}

TreeView::~TreeView()
{
	
}

TreeViewModel* TreeView::model()
{
	return (TreeViewModel*)QTreeView::model();
}

void TreeView::init()
{
	if (!check())
	{
		return;
	}
	m_treeViewDelegate->setParent(this);
	setItemDelegate(m_treeViewDelegate);
	setModel(m_treeViewModel);
	setHeader(m_treeViewHeader);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setFrameStyle(QFrame::NoFrame);
	//È¥µô×ó²à¿Õ°×
	setIndentation(0);
	setEditTriggers(TreeView::NoEditTriggers);
}

bool TreeView::check()
{
	return m_treeViewDelegate != nullptr &&
		m_treeViewModel != nullptr &&
		m_treeViewHeader != nullptr;
}