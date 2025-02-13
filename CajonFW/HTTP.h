#ifndef _HTTP_H_
#define _HTTP_H_

#include <queue.h>
#include "reqid.h"


const char* playlisthtmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html>
        <head>
            <meta charset='UTF-8'>
            <title> Music List </title>
            <style>
                body { font-family: Arial, sans-serif; text-align: center; background-color: #2c3e50; color: white;}
                .container { margin-top: 50px; }
                .header {display: flex; justify-content: space-between; align-items: center; padding: 10px; background-color: #2C3E50; color: white;}
                .menu-button { cursor: pointer; font-size: 24px; }
                .menu { display: none; position: absolute; top: 50px; right: 10px; background-color: #34495E; padding: 10px; border-radius: 5px; }
                .menu-item { padding: 10px; color: white; cursor: pointer; }
                .menu-item:hover { background-color: #2c3e50;}
                .track-name { font-size: 24px; margin-top: 10px; }
                .controls { display: flex; justify-content: center; align-items: center; margin-top: 20px;}
                .progress { width: 80%; margin: 20px auto; }
                .track-list { text-align: left; margin: 0 auto; width: 80%; }
                .track { font: 18px; padding: 10px; border-bottom: 1px solid #34495e; cursor: pointer;}
                .track:hover { background-color: #34495e; }
            </style>
        </head>
    
        <body>
        <!-- メニューリスト画面 -->
            <div id="menuListScreen" class="container">
                <div class="header">
                    <div class="icon">🎵</div>
                    <div class="menu-button" onclick="toggleMenu()">三</div>
                </div>
                <!-- メニューバーガー押した後の項目 -->
                <div class="menu" id="menu">
                    <div class="menu-item" onclick="goToHome()">スタート画面に戻る</div>
                    <div class="menu-item">メニュー項目2</div>
                </div>
                <div class="track-name">曲名</div>
                <!-- シークバー（見た目のみ） -->
                <input type="range" id="progressBar" class="progress-bar" value="0" max="100">
                <!-- コントロールボタン類 -->
                <div class="controls">
                    <button class="button-prev" onclick="prevTrack()">&#x23EE;</button>
                    <button class="button-start-stop" id="StartStop" onclick="startStop()">&#x25b6;</button>
                    <button class="button-next" onclick="nextTrack()">&#x23ED;</button>
                    <!-- <button class="button" onclick="stopTrack()">&#x23F8;</button> -->
                </div>
                <div class="track-list">
                    <!-- Arduinoコード内では曲数分ループ回す? -->
                    <div class="track" onclick="playTrack('曲名1')">曲名1</div>
                    <div class="track" onclick="playTrack('曲名2')">曲名2</div>
                    <div class="track" onclick="playTrack('曲名3')">曲名3</div>
                </div>
            </div>
            <script>
                function toggleMenu() {
                    var menu = document.getElementById('menu');
                    if (menu.style.display === 'none' || menu.style.display === '') {
                        menu.style.display = 'block';
                    } else {
                        menu.style.display = 'none';
                    }
                }
    
                function goToHome() {
                    location.href = '/';
                }
    
                function prevTrack() {
                    fetch('/prevTrack');
                }
                function startTrack() {
                    fetch('/startTrack');
                }
                function nextTrack() {
                    fetch('/nextTrack');
                }
                function stopTrack() {
                    fetch('/stopTrack');
                }
                function playTrack(trackName) {
                    fetch('/playTrack?name=' + encodeURIComponent(trackName));
                }
                // 絵文字を文字コードで比較してオン/オフ切り替え
                function startStop() {
                    let icon = document.getElementById('StartStop');
                    if (icon.innerHTML.charCodeAt(0).toString(16)==='25b6') {
                        fetch('/stopTrack');
                        icon.innerHTML='&#x23F8';
                    } else {
                        fetch('/startTrack');
                        icon.innerHTML='&#x25b6';
                    }
                }
    
                document.getElementById('progressBar').addEventListener('input', function() {
                    fetch('/seek?value=' + this.value);
                })
            </script>
        </body>
    </html>
    )rawliteral";
    
    const char* indexhtmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html>
            <head>
                <meta charset="UTF-8">
                <title> Music Control </title>
                <style>
                    body { font-family: Arial, sans-serif; text-align: center; background-color: #2c3e50; color: white;}
                    .container { margin-top: 50px; }
                    .icon { font-size: 100px; }
                    .button { font-size: 24px; padding: 10px 20px; margin-top: 20px; }
                    .version { margin-top: 50px; font-size: 18; }
                </style>
            </head>
            <body>
                <div class="container">
                    <div class="icon">🎵</div>
                    <button class="button" onclick="location.href='/playlist.html'">Music Start</button>
                    <div="version">V1.0.0</div>
                </div>
            </body>
        </html>
        )rawliteral";    

#endif
