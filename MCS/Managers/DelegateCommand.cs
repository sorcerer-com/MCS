﻿using System;
using System.Windows.Input;

namespace MCS.Managers
{
    public class DelegateCommand : ICommand
    {
        private Action<object> callback;
        private Predicate<object> canExecuteCallback;

        public event EventHandler CanExecuteChanged;


        public DelegateCommand(Action<object> callback)
        {
            this.callback = callback;
            this.canExecuteCallback = (p => true);
        }

        public DelegateCommand(Action<object> callback, Predicate<object> canExecuteCallback)
        {
            this.callback = callback;
            this.canExecuteCallback = canExecuteCallback ?? (p => true);
        }


        public void UpdateCanExecute()
        {
            this.OnCanExecuteChanged(EventArgs.Empty);
        }

        public bool CanExecute(object parameter)
        {
            return this.canExecuteCallback(parameter);
        }

        public void Execute(object parameter)
        {
            this.callback(parameter);
        }


        protected virtual void OnCanExecuteChanged(EventArgs eventArgs)
        {
            if (this.CanExecuteChanged != null)
            {
                this.CanExecuteChanged(this, eventArgs);
            }
        }

    }
}
