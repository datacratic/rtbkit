#!/bin/sh
curl -H "Accept: application/json" -X POST -d @http_config.json http://127.0.0.1:9986/v1/agents/my_http_config/config
