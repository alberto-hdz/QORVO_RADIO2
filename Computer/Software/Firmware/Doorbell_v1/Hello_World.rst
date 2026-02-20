HelloWorld
==========

Introduction
------------

The Hello World reference application demonstrates the basic logging
functionality of the QPG6200 Development Kit.

IoT Carrier Board
-----------------

Components used in this example are marked on the diagram below.

.. raw:: html

   <link rel="stylesheet" href="../../_static/board_layout/rst_helpers/rst_board_layout.css">
   <script type="text/javascript" src="../../_static/board_layout/rst_helpers/rst_board_layout.js"></script>
   <script>
     // Title to display in the new window interactive view
     const newWindowTitle = "Hello World Example Application"
     // List of elements to show. Use on elements hidden in the SVG, like tags.
     const labelsToShow = ['GPIO11', 'GPIO9', 'GPIO8', 'D6', 'J35']
     // List of elements to hide. Use on visible elements, like Effects or Components.
     const labelsToHide = []
     // Execute JS after page loads completely
     document.addEventListener('DOMContentLoaded', function() {
         // Display and hide SVG elements
         modifySVG(labelsToShow, labelsToHide);
         // Modify New Window link to pass arguments that show/display elements
         newWindowUrl(labelsToShow, labelsToHide, newWindowTitle);
     });
   </script>
   <div class="svg-container">
        <object class="svg-content" type="image/svg+xml" data="../../_static/board_layout/gp_p346_hw_20344_iot_carrier_board_pcb11.svg" id="svgObject"></object>
        <div>
            <span class="svglink svglink-green">
               <a id="board-layout-new-window" class="svglink-button" href="../../_static/board_layout/interactive_view/board_layout.html" target="_blank"> Interactive view </a>
            </span>
            <span class="svglink svglink-blue">
               <a class="svglink-button" href="../../_static/board_layout/gp_p346_hw_20344_iot_carrier_board_pcb11.svg" download> Download </a>
            </span>
        </div>

   </div>



QPG6200 GPIO Configurations
---------------------------

========= ========= ============ =====================
GPIO Name Direction Connected to Comments
========= ========= ============ =====================
GPIO11    Output    D6           RGB LED
GPIO9     Output    J35          Configured as UART TX
GPIO8     Input     J35          Configured as UART RX
========= ========= ============ =====================

Usage
-----

The application every second:

- Prints “Hello World”
- Blinks the LED

For a guide to enable serial logging, please refer to the `serial
logging section <../../sphinx/docs/QUICKSTART.rst#serial-logging>`__.

Once serial logging is correctly setup, after resetting the programmed
QPG6200 with the application, you will see “Hello world” is printed out
periodically which will be similar as below:

::

   Hello world
   Hello world
   Hello world
   Hello world
   Hello world
   Hello world
