/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Dave Coleman */

// SA
#include <moveit_setup_srdf_plugins/end_effectors_widget.hpp>
#include <moveit_setup_framework/qt/helper_widgets.hpp>

// Qt
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace moveit_setup
{
namespace srdf_setup
{
// ******************************************************************************************
// Constructor
// ******************************************************************************************
void EndEffectorsWidget::onInit()
{
  // Basic widget container
  QVBoxLayout* layout = new QVBoxLayout();

  // Top Header Area ------------------------------------------------

  auto header = new HeaderWidget("Define End Effectors",
                                 "Setup your robot's end effectors. These are planning groups "
                                 "corresponding to grippers or tools, attached to a parent "
                                 "planning group (an arm). The specified parent link is used as the "
                                 "reference frame for IK attempts.",
                                 this);
  layout->addWidget(header);

  // Create contents screens ---------------------------------------

  effector_list_widget_ = createContentsWidget();
  effector_edit_widget_ = createEditWidget();

  // Create stacked layout -----------------------------------------
  stacked_widget_ = new QStackedWidget(this);
  stacked_widget_->addWidget(effector_list_widget_);  // screen index 0
  stacked_widget_->addWidget(effector_edit_widget_);  // screen index 1
  layout->addWidget(stacked_widget_);

  // Finish Layout --------------------------------------------------
  setLayout(layout);
}

// ******************************************************************************************
// Create the main content widget
// ******************************************************************************************
QWidget* EndEffectorsWidget::createContentsWidget()
{
  // Main widget
  QWidget* content_widget = new QWidget(this);

  // Basic widget container
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Table ------------ ------------------------------------------------

  data_table_ = new QTableWidget(this);
  data_table_->setColumnCount(4);
  data_table_->setSortingEnabled(true);
  data_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  connect(data_table_, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(editDoubleClicked(int, int)));
  connect(data_table_, SIGNAL(cellClicked(int, int)), this, SLOT(previewClicked(int, int)));
  layout->addWidget(data_table_);

  // Set header labels
  QStringList header_list;
  header_list.append("End Effector Name");
  header_list.append("Group Name");
  header_list.append("Parent Link");
  header_list.append("Parent Group");
  data_table_->setHorizontalHeaderLabels(header_list);

  // Bottom Buttons --------------------------------------------------

  QHBoxLayout* controls_layout = new QHBoxLayout();

  // Spacer
  controls_layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

  // Edit Selected Button
  btn_edit_ = new QPushButton("&Edit Selected", this);
  btn_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  btn_edit_->setMaximumWidth(300);
  btn_edit_->hide();  // show once we know if there are existing poses
  connect(btn_edit_, SIGNAL(clicked()), this, SLOT(editSelected()));
  controls_layout->addWidget(btn_edit_);
  controls_layout->setAlignment(btn_edit_, Qt::AlignRight);

  // Delete Selected Button
  btn_delete_ = new QPushButton("&Delete Selected", this);
  connect(btn_delete_, SIGNAL(clicked()), this, SLOT(deleteSelected()));
  controls_layout->addWidget(btn_delete_);
  controls_layout->setAlignment(btn_delete_, Qt::AlignRight);

  // Add end effector Button
  QPushButton* btn_add = new QPushButton("&Add End Effector", this);
  btn_add->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  btn_add->setMaximumWidth(300);
  connect(btn_add, SIGNAL(clicked()), this, SLOT(showNewScreen()));
  controls_layout->addWidget(btn_add);
  controls_layout->setAlignment(btn_add, Qt::AlignRight);

  // Add layout
  layout->addLayout(controls_layout);

  // Set layout -----------------------------------------------------
  content_widget->setLayout(layout);

  return content_widget;
}

// ******************************************************************************************
// Create the edit widget
// ******************************************************************************************
QWidget* EndEffectorsWidget::createEditWidget()
{
  // Main widget
  QWidget* edit_widget = new QWidget(this);
  // Layout
  QVBoxLayout* layout = new QVBoxLayout();

  // Simple form -------------------------------------------
  QFormLayout* form_layout = new QFormLayout();
  // form_layout->setContentsMargins( 0, 15, 0, 15 );
  form_layout->setRowWrapPolicy(QFormLayout::WrapAllRows);

  // Name input
  effector_name_field_ = new QLineEdit(this);
  form_layout->addRow("End Effector Name:", effector_name_field_);

  // Group input
  group_name_field_ = new QComboBox(this);
  group_name_field_->setEditable(false);
  form_layout->addRow("End Effector Group:", group_name_field_);
  connect(group_name_field_, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(previewClickedString(const QString&)));

  // Parent Link input
  parent_name_field_ = new QComboBox(this);
  parent_name_field_->setEditable(false);
  form_layout->addRow("Parent Link (usually part of the arm):", parent_name_field_);

  // Parent Group input
  parent_group_name_field_ = new QComboBox(this);
  parent_group_name_field_->setEditable(false);
  form_layout->addRow("Parent Group (optional):", parent_group_name_field_);

  layout->addLayout(form_layout);

  // Bottom Buttons --------------------------------------------------

  QHBoxLayout* controls_layout = new QHBoxLayout();
  controls_layout->setContentsMargins(0, 25, 0, 15);

  // Spacer
  controls_layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

  // Save
  btn_save_ = new QPushButton("&Save", this);
  btn_save_->setMaximumWidth(200);
  connect(btn_save_, SIGNAL(clicked()), this, SLOT(doneEditing()));
  controls_layout->addWidget(btn_save_);
  controls_layout->setAlignment(btn_save_, Qt::AlignRight);

  // Cancel
  btn_cancel_ = new QPushButton("&Cancel", this);
  btn_cancel_->setMaximumWidth(200);
  connect(btn_cancel_, SIGNAL(clicked()), this, SLOT(cancelEditing()));
  controls_layout->addWidget(btn_cancel_);
  controls_layout->setAlignment(btn_cancel_, Qt::AlignRight);

  // Add layout
  layout->addLayout(controls_layout);

  // Set layout -----------------------------------------------------
  edit_widget->setLayout(layout);

  return edit_widget;
}

// ******************************************************************************************
// Show edit screen for creating a new effector
// ******************************************************************************************
void EndEffectorsWidget::showNewScreen()
{
  // Remember that this is a new effector
  current_edit_effector_.clear();

  // Clear previous data
  effector_name_field_->setText("");
  parent_name_field_->clearEditText();
  group_name_field_->clearEditText();         // actually this just chooses first option
  parent_group_name_field_->clearEditText();  // actually this just chooses first option

  // Switch to screen
  stacked_widget_->setCurrentIndex(1);

  // Announce that this widget is in modal mode
  Q_EMIT setModalMode(true);
}

// ******************************************************************************************
// Edit whatever element is selected
// ******************************************************************************************
void EndEffectorsWidget::editDoubleClicked(int /*row*/, int /*column*/)
{
  editSelected();
}

// ******************************************************************************************
// Preview whatever element is selected
// ******************************************************************************************
void EndEffectorsWidget::previewClicked(int /*row*/, int /*column*/)
{
  // Get list of all selected items
  QList<QTableWidgetItem*> selected = data_table_->selectedItems();

  // Check that an element was selected
  if (selected.empty())
    return;

  // Find the selected in datastructure
  srdf::Model::EndEffector* effector = getEndEffector(selected[0]->text().toStdString());

  // Unhighlight all links
  rviz_panel_->unhighlightAll();

  // Highlight group
  rviz_panel_->highlightGroup(effector->component_group_);
}

// ******************************************************************************************
// Preview the planning group that is selected
// ******************************************************************************************
void EndEffectorsWidget::previewClickedString(const QString& name)
{
  // Don't highlight if we are on the overview end effectors screen. we are just populating drop down box
  if (stacked_widget_->currentIndex() == 0)
    return;

  // Unhighlight all links
  rviz_panel_->unhighlightAll();

  // Highlight group
  rviz_panel_->highlightGroup(name.toStdString());
}

// ******************************************************************************************
// Edit whatever element is selected
// ******************************************************************************************
void EndEffectorsWidget::editSelected()
{
  // Get list of all selected items
  QList<QTableWidgetItem*> selected = data_table_->selectedItems();

  // Check that an element was selected
  if (selected.empty())
    return;

  // Get selected name and edit it
  edit(selected[0]->text().toStdString());
}

// ******************************************************************************************
// Edit effector
// ******************************************************************************************
void EndEffectorsWidget::edit(const std::string& name)
{
  // Remember what we are editing
  current_edit_effector_ = name;

  // Find the selected in datastruture
  srdf::Model::EndEffector* effector = getEndEffector(name);

  // Set effector name
  effector_name_field_->setText(effector->name_.c_str());

  // Set effector parent link
  int index = parent_name_field_->findText(effector->parent_link_.c_str());
  if (index == -1)
  {
    QMessageBox::critical(this, "Error Loading", "Unable to find parent link in drop down box");
    return;
  }
  parent_name_field_->setCurrentIndex(index);

  // Set group:
  index = group_name_field_->findText(effector->component_group_.c_str());
  if (index == -1)
  {
    QMessageBox::critical(this, "Error Loading", "Unable to find group name in drop down box");
    return;
  }
  group_name_field_->setCurrentIndex(index);

  // Set parent group:
  index = parent_group_name_field_->findText(effector->parent_group_.c_str());
  if (index == -1)
  {
    QMessageBox::critical(this, "Error Loading", "Unable to find parent group name in drop down box");
    return;
  }
  parent_group_name_field_->setCurrentIndex(index);

  // Switch to screen
  stacked_widget_->setCurrentIndex(1);

  // Announce that this widget is in modal mode
  Q_EMIT setModalMode(true);
}

// ******************************************************************************************
// Populate the combo dropdown box with avail group names
// ******************************************************************************************
void EndEffectorsWidget::loadGroupsComboBox()
{
  // Remove all old groups
  group_name_field_->clear();
  parent_group_name_field_->clear();
  parent_group_name_field_->addItem("");  // optional setting

  // Add all group names to combo box
  for (const std::string& group_name : setup_step_.getGroupNames())
  {
    group_name_field_->addItem(group_name.c_str());
    parent_group_name_field_->addItem(group_name.c_str());
  }
}

// ******************************************************************************************
// Populate the combo dropdown box with avail parent links
// ******************************************************************************************
void EndEffectorsWidget::loadParentComboBox()
{
  // Remove all old groups
  parent_name_field_->clear();

  // Get all links in robot model
  // Add all links to combo box
  for (const std::string& link_name : setup_step_.getLinkNames())
  {
    parent_name_field_->addItem(link_name.c_str());
  }
}

// ******************************************************************************************
// Find the associated data by name
// ******************************************************************************************
srdf::Model::EndEffector* EndEffectorsWidget::getEndEffector(const std::string& name)
{
  srdf::Model::EndEffector* searched_group = setup_step_.find(name);

  // Check if effector was found
  if (searched_group == nullptr)  // not found
  {
    QMessageBox::critical(this, "Error Saving", "An internal error has occurred while saving. Quitting.");
    QApplication::quit();
  }

  return searched_group;
}

// ******************************************************************************************
// Delete currently editing item
// ******************************************************************************************
void EndEffectorsWidget::deleteSelected()
{
  // Get list of all selected items
  QList<QTableWidgetItem*> selected = data_table_->selectedItems();

  // Check that an element was selected
  if (selected.empty())
    return;

  // Get selected name and edit it
  current_edit_effector_ = selected[0]->text().toStdString();

  // Confirm user wants to delete group
  if (QMessageBox::question(this, "Confirm End Effector Deletion",
                            QString("Are you sure you want to delete the end effector '")
                                .append(current_edit_effector_.c_str())
                                .append("'?"),
                            QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
  {
    return;
  }

  setup_step_.remove(current_edit_effector_);

  // Reload main screen table
  loadDataTable();
}

// ******************************************************************************************
// Save editing changes
// ******************************************************************************************
void EndEffectorsWidget::doneEditing()
{
  // Get a reference to the supplied strings
  const std::string effector_name = effector_name_field_->text().toStdString();

  // Check that name field is not empty
  if (effector_name.empty())
  {
    QMessageBox::warning(this, "Error Saving", "A name must be specified for the end effector!");
    return;
  }

  // Check that a group was selected
  if (group_name_field_->currentText().isEmpty())
  {
    QMessageBox::warning(this, "Error Saving", "A group that contains the links of the end-effector must be chosen!");
    return;
  }

  // Check that a parent link was selected
  if (parent_name_field_->currentText().isEmpty())
  {
    QMessageBox::warning(this, "Error Saving", "A parent link must be chosen!");
    return;
  }

  if (!parent_group_name_field_->currentText().isEmpty())
  {
    if (!setup_step_.isLinkInGroup(parent_name_field_->currentText().toStdString(),
                                   parent_group_name_field_->currentText().toStdString()))
    {
      QMessageBox::warning(this, "Error Saving",
                           QString::fromStdString("The specified parent group '" +
                                                  parent_group_name_field_->currentText().toStdString() +
                                                  "' must contain the specified parent link '" +
                                                  parent_name_field_->currentText().toStdString() + "'."));
      return;
    }
  }

  // Save the new effector name or create the new effector ----------------------------

  try
  {
    srdf::Model::EndEffector* searched_data = setup_step_.get(effector_name, current_edit_effector_);
    // Copy name data ----------------------------------------------------
    setup_step_.setProperties(searched_data, parent_name_field_->currentText().toStdString(),
                              group_name_field_->currentText().toStdString(),
                              parent_group_name_field_->currentText().toStdString());
  }
  catch (const std::runtime_error& e)
  {
    QMessageBox::warning(this, "Error Saving", e.what());
    return;
  }

  // Finish up ------------------------------------------------------

  // Reload main screen table
  loadDataTable();

  // Switch to screen
  stacked_widget_->setCurrentIndex(0);

  // Announce that this widget is not in modal mode
  Q_EMIT setModalMode(false);
}

// ******************************************************************************************
// Cancel changes
// ******************************************************************************************
void EndEffectorsWidget::cancelEditing()
{
  // Switch to screen
  stacked_widget_->setCurrentIndex(0);

  // Re-highlight the currently selected end effector group
  previewClicked(0, 0);  // the number parameters are actually meaningless

  // Announce that this widget is not in modal mode
  Q_EMIT setModalMode(false);
}

// ******************************************************************************************
// Load the end effectors into the table
// ******************************************************************************************
void EndEffectorsWidget::loadDataTable()
{
  // Disable Table
  data_table_->setUpdatesEnabled(false);  // prevent table from updating until we are completely done
  data_table_->setDisabled(true);         // make sure we disable it so that the cellChanged event is not called
  data_table_->clearContents();

  const auto& end_effectors = setup_step_.getEndEffectors();

  // Set size of datatable
  data_table_->setRowCount(end_effectors.size());

  // Loop through every end effector
  int row = 0;
  for (const auto& eef : end_effectors)
  {
    // Create row elements
    QTableWidgetItem* data_name = new QTableWidgetItem(eef.name_.c_str());
    data_name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QTableWidgetItem* group_name = new QTableWidgetItem(eef.component_group_.c_str());
    group_name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QTableWidgetItem* parent_name = new QTableWidgetItem(eef.parent_link_.c_str());
    group_name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QTableWidgetItem* parent_group_name = new QTableWidgetItem(eef.parent_group_.c_str());
    parent_group_name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    // Add to table
    data_table_->setItem(row, 0, data_name);
    data_table_->setItem(row, 1, group_name);
    data_table_->setItem(row, 2, parent_name);
    data_table_->setItem(row, 3, parent_group_name);

    // Increment counter
    ++row;
  }

  // Re-enable
  data_table_->setUpdatesEnabled(true);  // prevent table from updating until we are completely done
  data_table_->setDisabled(false);       // make sure we disable it so that the cellChanged event is not called

  // Resize header
  data_table_->resizeColumnToContents(0);
  data_table_->resizeColumnToContents(1);
  data_table_->resizeColumnToContents(2);
  data_table_->resizeColumnToContents(3);

  // Show edit button if applicable
  if (!end_effectors.empty())
    btn_edit_->show();
}

// ******************************************************************************************
// Called when setup assistant navigation switches to this screen
// ******************************************************************************************
void EndEffectorsWidget::focusGiven()
{
  // Show the current effectors screen
  stacked_widget_->setCurrentIndex(0);

  // Load the data to the tree
  loadDataTable();

  // Load the avail groups to the combo box
  loadGroupsComboBox();
  loadParentComboBox();
}

}  // namespace srdf_setup
}  // namespace moveit_setup

#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(moveit_setup::srdf_setup::EndEffectorsWidget, moveit_setup::SetupStepWidget)
