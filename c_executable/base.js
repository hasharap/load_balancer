const { initializeControllers } = require('./lib/controller.js');

(async () => {
  const { createMachine, interpret, assign, actions } = require('xstate');
  const { gpioController, ledController } = await initializeControllers();
  // const { gpioController, ledController } = require('./lib/controller.js');
  const { DisplayManager, liveData, statData } = require('./lib/display.js');
  const { MCUManager } = require('./lib/mcu.js');
  const { logger } = require('./lib/log.js');
  const { PipeLog } = require('./lib/pipelogs');
  const config = require('./config.json');
  const Database = require('./lib/db.js');
  const nBoot = require('./lib/nboot.js');

  const mcu = new MCUManager();
  const pipe = new PipeLog();
  const nb = new nBoot();

  const tickInterval = 1000; // 1 second
  let L2Child = null;
  let L2MainState = null;

  // Define the L2 state machine
  function createL2StateMachine(db, display, liveDMG, netDMG, config) {
    return createMachine({
      id: 'L2',
      initial: "setup",
      predictableActionArguments: true,
      context: {
        stratCalled: false,
        errorCode: 0,
        powerError: '',
        db: db,

        elapsedTime: 0,
        prvEnergy: 0,
        nbootDB: 0,
        nbootLog: 0,
        stop: false,

        liveDMG: liveDMG,
        netDMG: netDMG,
        config: config,

        interval: null,
        elapsedTime: 0,
        hours: 0,
        minutes: 0,
        seconds: 0,

        consumedEnergy: 0,
        prvEnergy: 0,
        current: 0,
        energy: 0,
        voltage: 0,
        power: 0,
        soc: 0

      },
      states: {

        setup: {
          invoke: {
            src: (context) => new Promise(async (resolve, reject) => {
              try {
                const elapsedTime = await context.db.read('elapsedTime');
                const prvEnergy = await context.db.read('energy');
                const stop = await context.db.read('stop');
                const nbootDB = await context.db.read('nboot');
                const nbootLog = await nb.getcount();

                // Resolve the promise with the data
                resolve({ elapsedTime, prvEnergy, stop, nbootDB, nbootLog });
              } catch (error) {
                // Handle any errors that occurred during the async operations
                logger.error(`Error Encountered at setup State: ${error}`);
              }
            }),
            onDone: {
              target: 'checking',
              actions: assign({
                elapsedTime: (_, event) => event.data.elapsedTime.data,
                prvEnergy: (_, event) => event.data.prvEnergy.data,
                stop: (_, event) => event.data.stop.data,
                nbootDB: (_, event) => parseInt(event.data.nbootDB.data),
                nbootLog: (_, event) => parseInt(event.data.nbootLog)
              })
            },
            onError: {
              // Define how to handle the error here
              actions: (context, event) => {
                logger.error(`Error in setup state: ${event.data}`);
              },
              after: {
                10000: 'setup'  // Transition to 'init' state after 10 seconds
              }
            }
          }
        },

        checking: {
          always: [
            {
              target: 'T',
              actions: assign({
                prvEnergy: () => 0
              }),
              cond: (context) => context.stop === false && context.nbootDB === context.nbootLog
            },
            {
              target: 'T'
            }
          ]
        },

        T: {
          initial: 'idle',
          on: {
            UPDATEL2: [
              {
                actions: () => {
                  mcu.writeMCUL2Data("IDLE");
                },
                cond: (context, event) => event.activityState.ConnectorState === '0' && event.state === 'F' && event.netRequest.includes('IDLE')
              },
              {
                target: 'A1',
                cond: (context, event) => event.state === 'A1' && event.powerError === ""
              },
              {
                target: 'B1',
                actions: () => {
                  mcu.writeMCUL2Data("PRE_START")
                },
                cond: (context, event) => event.state === 'B1' && event.powerError === ""
              },
              {
                target: 'C1',
                actions: () => {
                  mcu.writeMCUL2Data("PRE_START")
                },
                cond: (context, event) => event.state === 'C1' && event.powerError === ""
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F' && event.powerError === ""
              }
            ]
          },
          states: {
            idle: {
              entry: () => {
                display.pageChange(67); // LOADING
                ledController.requestAction([1], ['Blink']);
              }
            }
          }
        },

        F: {
          initial: 'idle',
          on: {
            UPDATEL2: [
              {
                target: 'A1',
                cond: (context, event) => event.state === 'A1' && event.powerError === ""
              },
              {
                actions: () => {
                  mcu.writeMCUL2Data('STOP');
                },
                cond: (context, event) => event.state !== 'A1'
              }
            ]
          },
          states: {
            idle: {
              entry: (context) => {
                logger.info(`powerError, ${context.powerError}`);
                switch (context.powerError) {
                  case "Ground Fault":
                    context.errorCode = 1;
                    break;
                  case "Over Current Fault":
                    context.errorCode = 2;
                    break;
                  case "GFI Test Failed":
                    context.errorCode = 3;
                    break;
                  case "Stuck Contactor Error":
                    context.errorCode = 4;
                    break;
                  case "Under Voltage Error":
                    context.errorCode = 5;
                    break;
                  case "Over Voltage Error":
                    context.errorCode = 6;
                    break;
                  case "Over Temp":
                    context.errorCode = 7;
                    break;
                  case "CP Fault":
                    context.errorCode = 8;
                    break;
                  case "Diode failure":
                    context.errorCode = 9;
                    break;
                  case "Over Voltage":
                    context.errorCode = 10;
                    break;
                  case "Under Voltage":
                    context.errorCode = 11;
                    break;
                  case "Freq Fault":
                    context.errorCode = 12;
                    break;
                  case "Diode Faliure":
                    context.errorCode = 13;
                    break;
                  case "MCU Param Missmatch":
                    context.errorCode = 20;
                    break;
                  default:
                    context.errorCode = 0;
                }

                context.stratCalled = false;
                mcu.writeMCUL2Data('STOP');
                display.pageChange(75, { errorCode: context.errorCode });
                ledController.requestAction([1], ['OFF']);
              }
            }
          }
        },

        A1: {
          initial: 'init',
          on: {
            UPDATEL2: [
              {
                target: "B1",
                actions: (context) => {
                  mcu.writeMCUL2Data("PRE_START")
                },
                cond: (context, event) => event.state === 'B1'
              },
              {
                target: "B2",
                cond: (context, event) => event.state === 'B2'
              },
              {
                actions: (context) => {
                  mcu.writeMCUL2Data("PRE_START")
                },
                cond: (context, event) => event.state === 'C1'
              },
              {
                target: "C2",
                cond: (context, event) => event.state === 'C2'
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F'
              }
            ],
            FAILED: {
              target: '.failed'
            },
            IDLE: {
              target: '.init'
            }
          },
          states: {
            init: {
              invoke: [
                {
                  src: (context, event) => {
                    return context.db.read('transHist'); // db.read returns a promise
                  },
                  onDone: {
                    target: 'idle',
                    actions: (context, event) => {
                      (context.netDMG).Energy = event.data.Energy;
                      (context.netDMG).Cost = event.data.Cost;
                      (context.netDMG).Time = event.data.Time;
                    }
                  }
                }
              ]
            },
            idle: {
              invoke: {
                src: 'clearContextAndUpdateDB'
              },
              entry: (context) => {
                context.stratCalled = false;
                mcu.writeMCUL2Data('IDLE');
                display.pageChange(68); // LAST CHARGE
                ledController.requestAction([1], ['Blink']);
              }
            },
            failed: {
              entry: (context) => {
                mcu.writeMCUL2Data('IDLE');
                display.pageChange(71); // FAILED
                ledController.requestAction([1], ['Blink']);
              },
              on: {
                UPDATEL2: [
                  {
                    target: "#L2.B1.failed",
                    cond: (context, event) => event.state === 'B1'
                  },
                  {
                    target: "#L2.B2.failed",
                    cond: (context, event) => event.state === 'B2'
                  }
                ]
              },
              after: {
                3000: 'init'  // Transition to 'init' state after 3 seconds
              }
            }
          }
        },

        A2: {
          initial: 'idle',
          states: {
            delayed: {
              after: {
                5000: 'idle'
              }
            },
            idle: {
              initial: 'init',
              entry: (context) => {
                display.pageChange(68);
                ledController.requestAction([1], ['Blink']);
              },
              states: {
                init: {
                  invoke: {
                    src: 'StopTransactionReq',
                    onDone: {
                      target: 'idle'
                    },
                    onError: {
                      target: '' // error handling state
                    }
                  }
                },
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange(74); // UNPLUG your EV
                    ledController.requestAction([1], ['Blink']);
                  }
                }
              },
              on: {
                UPDATEL2: [
                  {
                    target: "#L2.A1",
                    cond: (context, event) => event.state === 'A1'
                  },
                  {
                    target: "#L2.B2",
                    cond: (context, event) => event.state === 'B2'
                  },
                  {
                    target: '#L2.F',
                    cond: (context, event) => event.state === 'F'
                  },
                ],
              }
            }
          }
        },

        B1: {
          initial: 'idle',
          on: {
            UPDATEL2: [
              {
                target: "A1",
                cond: (context, event) => event.state === 'A1'
              },
              {
                target: "B2",
                cond: (context, event) => event.state === 'B2'
              },
              {
                target: "C2",
                cond: (context, event) => event.state === 'C2'
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F'
              },
            ],
            IDLE: {
              target: '.idle'
            },
            FAILED: {
              target: '.failed'
            }
          },
          states: {
            idle: {
              entry: (context) => {
                mcu.writeMCUL2Data('PRE_START');
                ledController.requestAction([1], ['ON']);
              }
            },
            failed: {
              entry: (context) => {
                mcu.writeMCUL2Data('IDLE');
                display.pageChange(71); // FAILED
                ledController.requestAction([1], ['Blink']);
              },
              on: {
                UPDATEL2: [
                  {
                    target: "#L2.A1.failed",
                    cond: (context, event) => event.state === 'A1'
                  },
                  {
                    target: "#L2.B2.failed",
                    cond: (context, event) => event.state === 'B2'
                  }
                ]
              },
              after: {
                3000: 'idle'  // Transition to 'dis' state after 3 seconds
              }
            },
          }
        },

        B2: {
          initial: 'idle',
          on: {
            UPDATEL2: [
              {
                target: "A2",
                actions: (context) => {
                  ledController.requestAction([1], ['ON']);
                },
                cond: (context, event) => event.state === 'A2'
              },
              {
                target: "B1",
                cond: (context, event) => event.state === 'B1'
              },
              {
                target: "C2",
                cond: (context, event) => event.state === 'C2'
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F'
              },
            ],
            BTN1: {
              target: '.stop'
            },
            STOP: {
              target: '.stop'
            },
            FAILED: {
              target: '.failed'
            }
          },
          states: {
            idle: {
              entry: (context) => {
                display.pageChange(77); // WAITING TO CAHRGE
                ledController.requestAction([1], ['ON']);
              },
              after: {
                60000: 'unplug'  // Transition to 'unplug' state after 60 seconds
              }
            },
            stop: {
              initial: 'idle',
              states: {
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange();
                    ledController.requestAction([1], ['ON']);
                  }
                }
              }
            },
            unplug: {
              initial: 'idle',
              states: {
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange('L', 11); // Unplug Display
                    ledController.requestAction([1], ['Blink']);
                  }
                }
              }
            },
            failed: {
              entry: (context) => {
                mcu.writeMCUL2Data('STOP');
                display.pageChange(71); // FAILED
                ledController.requestAction([1], ['Blink']);
              },
              on: {
                UPDATEL2: [
                  {
                    target: "#L2.A1.failed",
                    cond: (context, event) => event.state === 'A1'
                  },
                  {
                    target: "#L2.B1.failed",
                    cond: (context, event) => event.state === 'B1'
                  }
                ]
              },
              after: {
                3000: 'idle'  // Transition to 'idle' state after 3 seconds
              }
            }
          }
        },

        C1: {
          initial: 'idle',
          on: {
            UPDATEL2: [
              {
                target: "A1",
                cond: (context, event) => event.state === 'A1'
              },
              {
                target: "B1",
                actions: (context) => {
                  mcu.writeMCUL2Data('IDLE');
                  display.pageChange(78); // LAST CHARGE OPTION 2
                  ledController.requestAction([1], ['Blink']);
                },
                cond: (context, event) => event.state === 'B1'
              },
              {
                target: "C2",
                cond: (context, event) => event.state === 'C2'
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F'
              },
            ],
            BTN1: {
              target: '.stop'
            },
            STOP: {
              target: '.stop'
            },
            IDLE: {
              target: '.unplug'
            }
          },
          states: {
            idle: {
              after: {
                60000: 'unplug'  // Transition to 'unplug' state after 60 seconds
              }
            },
            stop: {
              initial: 'idle',
              states: {
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange(74); // UNPLUG your EV
                    ledController.requestAction([1], ['Blink']);
                  }
                }
              }
            },
            unplug: {
              initial: 'idle',
              states: {
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange(74); // UNPLUG your EV
                    ledController.requestAction([1], ['Blink']);
                  }
                }
              }
            }
          }
        },

        C2: {
          initial: 'idle',
          invoke: [
            {
              src: (context) => (callback) => {
                context.interval = setInterval(() => {
                  callback('TICK');
                }, tickInterval);

                return () => clearInterval(context.interval); // Cleanup on state exit or when machine stops
              }
            }
          ],
          on: {
            TICK: {
              actions: (context, event) => {
                // Increment elapsedTime by tickInterval (converted to seconds)
                context.elapsedTime += tickInterval / 1000;
                context.hours = Math.floor(context.elapsedTime / 3600);
                context.minutes = Math.floor((context.elapsedTime % 3600) / 60);

                context.db.update('elapsedTime', { data: context.elapsedTime });
                (context.liveDMG).elapsedTime = parseInt(context.elapsedTime);
              }
            },
            CAL: {
              actions: [
                assign({
                  current: (context, event) => event.current,
                  energy: (context, event) => event.energy,
                  power: (context, event) => event.power,
                  voltage: (context, event) => event.voltage,
                  soc: (context, event) => event.soc
                }),
                (context, event) => {
                  context.liveDMG.Power = (context.power).toFixed(1);
                  context.liveDMG.current = (context.current).toFixed(1);
                  try {
                    context.consumedEnergy = context.prvEnergy + context.energy;
                    context.liveDMG.Energy = Math.floor(context.consumedEnergy * 10) / 10;
                    context.liveDMG.Cost = Number(0);
                    context.db.update('energy', { data: context.consumedEnergy });
                  } catch (error) {
                    logger.error(`Error encountered in MeterValues: ${error}`);
                  }
                }
              ]
            },
            UPDATEL2: [
              {
                target: "A1",
                actions: assign((context, event) => {
                  // Reset the stratCalled flag to false
                  return { stratCalled: false };
                }),
                cond: (context, event) => event.state === 'A1'
              },
              {
                target: "A2",
                actions: [
                  assign((context, event) => {
                    // Reset the stratCalled flag to false
                    return { stratCalled: false };
                  }),
                  (context, event) => {
                    // display.pageChange(74); // UNPLUG your EV
                    display.pageChange(67); // LOADING
                    ledController.requestAction([1], ['Blink']);
                  }
                ],
                cond: (context, event) => event.state === 'A2'
              },
              {
                target: "B2",
                actions: assign((context, event) => {
                  // Reset the stratCalled flag to false
                  return { stratCalled: false };
                }),
                cond: (context, event) => event.state === 'B2'
              },
              {
                target: "C1",
                actions: assign((context, event) => {
                  // Reset the stratCalled flag to false
                  return { stratCalled: false };
                }),
                cond: (context, event) => event.state === 'C1'
              },
              {
                target: 'F',
                cond: (context, event) => event.state === 'F'
              }
            ],
            BTN1: {
              target: '.stop'
            },
            // STOP: {
            //   target: '.stop'
            // },
            DONE_STOP: {
              actions: () => {
                display.pageChange(74); // UNPLUG your EV
              }
            }
          },
          states: {
            idle: {
              entry: (context) => {
                display.pageChange(73); // CHARGING
                ledController.requestAction([1], ['ON']);
              }
            },
            stop: {
              initial: 'init',
              states: {
                init: {
                  invoke: {
                    src: 'StopTransactionReq',
                    onDone: {
                      target: 'idle'
                    },
                    onError: {
                      target: '' // error handling state
                    }
                  }
                },
                idle: {
                  entry: (context) => {
                    mcu.writeMCUL2Data("STOP");
                    display.pageChange(74); // UNPLUG your EV
                    ledController.requestAction([1], ['Blink']);
                  }
                }
              }
            }
          }
        }
      },
      on: {
        UPDATEL2: [
          {
            actions: (context) => {
              if (!context.stratCalled) {
                mcu.writeMCUL2Data("START");
                context.stratCalled = true; // Set flag to true after calling
              }
            },
            cond: (context, event) => event.netRequest.includes('Start')
          },
          {
            actions: assign({
              powerError: (context, event) => event.powerError
            }),
            cond: (context, event) => event.powerError != ''
          }
        ],
        ERROR: {
          target: 'F',
          entry: (context, event) => {
            logger.error(`Error: ${event.error}`);
          }
        }
      }
    }, {
      actions: {},
      services: {
        StopTransactionReq: (context, event) => new Promise(async (resolve, reject) => {
          try {
            let consumedEnergy = await context.db.read('energy');
            let _elapsedTime = await context.db.read('elapsedTime');
            let elapsedTime = _elapsedTime.data < 0 ? 0 : Math.ceil(_elapsedTime.data);

            await context.db.update('transHist', {
              Time: elapsedTime,
              Cost: Number(context.cost).toFixed(2),
              Energy: Math.floor(consumedEnergy.data * 10) / 10
            });

            L2Child.send("DONE_STOP");
            resolve();
          } catch (error) {
            reject(error);
          }
        }),
        clearContextAndUpdateDB: (context) => new Promise(async (resolve, reject) => {
          try {
            // Clear context variables
            context.elapsedTime = 0;
            context.consumedEnergy = 0;
            context.prvEnergy = 0;
            context.energy = 0;
            context.current = 0;
            context.voltage = 0;
            context.power = 0;
            context.soc = 0;
            context.stop = false;

            // Perform DB updates
            await context.db.update('elapsedTime', { data: 0 });
            await context.db.update('energy', { data: 0 });
            await context.db.update('stop', { data: false });

            resolve();
          } catch (error) {
            reject(error);
          }
        })
      }
    });
  };

  async function initializeServices() {
    try {
      const dbM1 = new Database('dbM1');
      await dbM1.create('elapsedTime', { data: 0 });
      await dbM1.create('energy', { data: 0 });
      await dbM1.create('transHist', { Time: 0, Cost: 0, Energy: 0 });
      await dbM1.create('stop', { data: false });
      await dbM1.create('nboot', { data: 0 });

      await mcu.checkmcu('M1');
      console.log("MCU IINIT complete");

      const display = new DisplayManager(dbM1);

      const L2machine = createL2StateMachine(dbM1, display, liveData, statData, config.M1);
      L2Child = interpret(L2machine).start();
    } catch (error) {
      logger.error(error);
    }

    setInterval(() => {
      if (L2Child && L2Child.state) {
        const L2ChildState = JSON.stringify(L2Child.state.value);

        let desiredKeysL2Child = ['hours', 'minutes', 'elapsedTime', 'consumedEnergy', 'prvEnergy', 'current', 'energy', 'voltage', 'power'];

        const L2ChildContext = Object.entries(L2Child.state.context)
          .filter(([key, _]) => desiredKeysL2Child.includes(key))
          .reduce((obj, [key, value]) => ({ ...obj, [key]: value }), {});

        logger.info(`L2Child State: ${L2ChildState} |`);
        logger.info(`L2Child Context: ${JSON.stringify(L2ChildContext)} |`);
      }
    }, 1000);
  };

  gpioController.on("btnPressed", (topic) => {
    logger.info(`Btn Pressed: ${topic}`);
    L2Child.send({ type: topic });
  });

  mcu.on("L2Data", (data) => {
    // logger.info(`L2 Data: ${JSON.stringify(data)}`);
    if (L2Child) {
      L2Child.send({
        type: 'UPDATEL2',
        state: data.state,
        mainState: L2MainState !== undefined ? JSON.parse(L2MainState) : "",
        activityState: data.activityState,
        netRequest: data.netRequest,
        powerError: data.powerError
      });

      L2Child.send({
        type: 'CAL',
        current: Number(data.curr) / 10,
        energy: Number(data.energy) / 100,
        voltage: Number(data.volt) / 10,
        power: Number(data.powr) / 10,
        soc: 0
      });
    }

    const { rss, heapTotal, heapUsed } = process.memoryUsage();
    const stateL2 = PipeLog.colorize(`\u25B2 state L2: ${data.state}`, 'greenBackground');
    const midL2 = PipeLog.colorize(`mid:${data.msgId}`, 'yellowBackground');
    const conL2 = PipeLog.colorize(`con:${data.activityStateRaw[0]} cp:${data.activityStateRaw[1]} > ${data.cpval} chg:${data.activityStateRaw[2]}`, 'yellowBackground');
    const vcpL2 = `v:${Number(data.volt) / 10} cur:${Number(data.curr) / 100} pwr:${Number(data.powr) / 10} ene:${Number(data.energy) / 100}`;
    const cdata = `  ${Number(data.v1L2/10)} ${Number(data.v2L2/10)} ${Number(data.v3L2)/10} T:${Number(data.tempL2)}C FRQ:${data.frqL2}Hz`;
    const memUsageL2 = `rss:${(rss / 1048576).toFixed(1)}Mb heapTotal:${(heapTotal / 1048576).toFixed(1)}Mb heapUsed:${(heapUsed / 1048576).toFixed(1)}Mb`;
    const mcuReadBufferL2 = PipeLog.colorize(`MCU Read Buffer: ${data.message}`, 'cyanText');
    const bufOutL2 = PipeLog.colorize(`MCU Write Buffer:${data.bufOut}`, 'cyanText');
    const netRequestL2 = PipeLog.colorize(`\u25B2 netReq: ${data.netRequest}`, 'greenBackground');
    const netcmd = PipeLog.colorize(`\u25BC netCmd: ${data.netCmd}`, 'greenBackground');
    const powerErrorL2 = PipeLog.colorize(`powerError:${data.powerError}`, 'redBackground');

    const messageAnsiL2 = `
  ${stateL2} ${midL2} ${conL2} 
  ${powerErrorL2}
  ${vcpL2} 
  ${cdata} 
  ${netRequestL2}
  ${netcmd} 
  ____________________________________________________________
  ${memUsageL2}
  ${mcuReadBufferL2}
  ${bufOutL2}
  ____________________________________________________________
  ${L2MainState}
  `;
    pipe.write(messageAnsiL2);

  });

  mcu.on('noData', (data) => {
    if (data.parserType === 'timerL2' && L2Child) {
      logger.error(`L2 data not received past 10 seconds...`)
      L2Child.send({ type: 'ERROR', error: 'L2 data not received' });
    }
  });

  initializeServices();

  setInterval(() => {
    const memoryUsage = process.memoryUsage();
    logger.info(`Memory usage: ${JSON.stringify(memoryUsage)}`);
  }, 10000); // Log memory usage every 10 seconds

  const processCleanup = () => {
    gpioController.cleanupAndExit();
    pipe.gracefullkill();
  };

  // Attach the wrapper function to the process signals
  process.on('SIGINT', processCleanup);
  process.on('SIGTERM', processCleanup);
  process.on('exit', processCleanup);

})();