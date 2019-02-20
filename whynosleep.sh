#!/bin/sh
pmset -g assertions | grep "PreventUserIdleSystemSleep" | grep pid
