﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Windows.Forms;
using ArducopterConfigurator.PresentationModels;
using ArducopterConfigurator.views;
using ArducopterConfigurator.Views;

namespace ArducopterConfigurator
{
    public partial class mainForm : Form, IView<MainVm>
    {
        private MainVm _vm;

        public mainForm(MainVm vm)
        {
            InitializeComponent();
            SetDataContext(vm);
        }

        // IPresentationModel to IView map
        private readonly Dictionary<Type, Type> _viewMap = new Dictionary<Type, Type>
                     {
                         {typeof (FlightDataVm), typeof (FlightDataView)},
                         {typeof (TransmitterChannelsVm), typeof (TransmitterChannelsView)},
                         {typeof (CalibrationOffsetsDataVm), typeof (CalibrationView)},
                         {typeof (MotorCommandsVm), typeof (MotorCommandsView)},
                         {typeof (AcroModeConfigVm), typeof (AcroConfigView)},
                         {typeof (StableModeConfigVm), typeof (StableConfigView)},
                         {typeof (PositionHoldConfigVm), typeof (PositionHoldConfigView)},
                         {typeof (AltitudeHoldConfigVm), typeof (AltitudeHoldConfigView)},
                         {typeof (SerialMonitorVm), typeof (SerialMonitorView)},
                     };


        private Control CreateAndBindView(IPresentationModel model)
        {
            if (_viewMap.ContainsKey(model.GetType()))
            {
                var viewtype = _viewMap[model.GetType()];
                var view = Activator.CreateInstance(viewtype);

                var methodInfo = this.GetType().GetMethod("BindView");
                var closedBindViewMethod = methodInfo.MakeGenericMethod(model.GetType());
                return closedBindViewMethod.Invoke(this, new[] { model, view }) as Control;
            }
            return null;
        }

        // called via reflection above to close the T
        public Control BindView<T>(T model, IView<T> view) where T : IPresentationModel
        {
            view.SetDataContext(model);
            return view.Control;
        }

        protected void BindButtons(MainVm vm)
        {
            foreach (var c in this.Controls)
                if (c is Button)
                    (c as Button).Click += btn_Click;

            vm.PropertyChanged += CheckCommandStates;
            CheckCommandStates(this,new PropertyChangedEventArgs(string.Empty));
        }

        void CheckCommandStates(object sender, PropertyChangedEventArgs e)
        {
            foreach (var c in this.Controls)
            {
                var btn = c as Button;
                if (btn == null) continue;
                var cmd =btn.Tag as ICommand;
                if (cmd!=null)
                    btn.Enabled = cmd.CanExecute(null);
            }
        }

        protected void btn_Click(object sender, EventArgs e)
        {
            var cmd = (sender as Button).Tag as ICommand;

            if (cmd != null)
                if (cmd.CanExecute(null))
                    cmd.Execute(null);
        }

     
        #region Implementation of IView

        public void SetDataContext(MainVm model)
        {
            _vm = model;
            mainVmBindingSource.DataSource = model;

            foreach (var monitorVm in _vm.MonitorVms)
            {
                var tp = new TabPage(monitorVm.Name) {Tag = monitorVm};
                var control = CreateAndBindView(monitorVm);
                if (control != null)
                {
                    tp.Controls.Add(control);
                    control.Size = tp.ClientRectangle.Size;
                }
                tabCtrlMonitorVms.TabPages.Add(tp);
            }

            var tabVm = tabCtrlMonitorVms.SelectedTab.Tag as MonitorVm;
            _vm.Select(tabVm);

            UpdateConnectionStatusLabel();
            _vm.PropertyChanged += ((sender, e) => UpdateConnectionStatusLabel());

        }

        public Control Control
        {
            get { return this; }
        }

        #endregion

        private void tabCtrlConfigs_Selected(object sender, TabControlEventArgs e)
        {
            var control = e.TabPage.Controls[0];
            control.Size = e.TabPage.ClientRectangle.Size;
            var tabVm = e.TabPage.Tag as MonitorVm;
            _vm.Select(tabVm);
        }

     
        private void mainForm_SizeChanged(object sender, EventArgs e)
        {
            ResizeChildControls();
        }

        private void MainFormLoaded(object sender, EventArgs e)
        {
            ResizeChildControls();
            BindButtons(_vm);
        }

        private void ResizeChildControls()
        {
            foreach (TabPage tabPage in tabCtrlMonitorVms.TabPages)
            {
                var control = tabPage.Controls[0];
                control.Size = tabPage.ClientRectangle.Size;
            }
        }

        private void UpdateConnectionStatusLabel()
        {
            switch (_vm.ConnectionState)
            {
                case MainVm.SessionStates.Disconnected:
                     lblConnectionStatus.Text="Not Connected";
                    break;
                case MainVm.SessionStates.Connecting:
                     lblConnectionStatus.Text="Connecting...";
                    break;
                case MainVm.SessionStates.Connected:
                     lblConnectionStatus.Text="Connected - version " + _vm.ApmVersion;
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }
    }
}