# LVGL Page Management Library

This is a simple library for managing a stack of pages while using LVGL as UI library.

A page in this context is a group of widgets that work under a common function, share some state and are displayed in the same screen.

Pages are organized in a stack where only the top is active at any given moment. 
The active page receives events and reacts to them by changing the displayed content, the local state or by returning a message to the underlying system.