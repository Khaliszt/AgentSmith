// C:\FarfadetsCorp\AgentSmith\resources\web\terminal.js

(function() {
    'use strict';

    const fitAddon = new FitAddon.FitAddon();
    const webLinksAddon = new WebLinksAddon.WebLinksAddon();

    const terminal = new Terminal({
        cursorBlink: true,
        fontSize: 14,
        fontFamily: 'JetBrains Mono, Consolas, monospace',
        theme: {
            background: '#24273a',
            foreground: '#cad3f5'
        }
    });

    terminal.loadAddon(fitAddon);
    terminal.loadAddon(webLinksAddon);
    terminal.open(document.getElementById('terminal'));
    fitAddon.fit();

    // Handle resize
    window.addEventListener('resize', () => fitAddon.fit());

    // Handle terminal input -> send to C++
    terminal.onData(data => {
        window.chrome.webview.postMessage({ type: 'input', data: data });
    });

    // Handle terminal resize -> send to C++
    terminal.onResize(size => {
        window.chrome.webview.postMessage({
            type: 'resize',
            cols: size.cols,
            rows: size.rows
        });
    });

    // C++ -> Terminal communication
    window.terminalApi = {
        write: function(data) {
            terminal.write(data);
        },
        clear: function() {
            terminal.clear();
        },
        reset: function() {
            terminal.reset();
        },
        setTheme: function(theme) {
            terminal.options.theme = theme;
        },
        getSelection: function() {
            return terminal.getSelection();
        },
        selectAll: function() {
            terminal.selectAll();
        },
        clearSelection: function() {
            terminal.clearSelection();
        },
        focus: function() {
            terminal.focus();
        },
        resize: function(cols, rows) {
            terminal.resize(cols, rows);
        },
        scrollToBottom: function() {
            terminal.scrollToBottom();
        },
        scrollToTop: function() {
            terminal.scrollToTop();
        }
    };

    // Notify C++ that terminal is ready
    window.chrome.webview.postMessage({ type: 'ready' });
})();
