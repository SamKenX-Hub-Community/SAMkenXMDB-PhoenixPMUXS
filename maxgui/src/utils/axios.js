/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-05-22
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
import ax from 'axios'
import { handleNullStatusCode, defErrStatusHandler } from '@share/axios/handlers'
import router from '@rootSrc/router'

let controller = new AbortController()
const abortRequests = () => {
    controller.abort()
    // refresh the controller
    controller = new AbortController()
}

function getBaseHttp() {
    return ax.create({
        baseURL: '/',
        headers: {
            'X-Requested-With': 'XMLHttpRequest',
            'Content-Type': 'application/json',
            'Cache-Control': 'no-cache',
        },
    })
}

// axios instance for `/auth` endpoint
const authHttp = getBaseHttp()
authHttp.interceptors.request.use(
    config => ({ ...config }),
    error => Promise.reject(error)
)

// axios instance for all endpoints except `/sql`
function http(store) {
    let http = getBaseHttp()

    http.interceptors.request.use(
        config => ({ ...config, signal: controller.signal }),
        error => Promise.reject(error)
    )
    http.interceptors.response.use(
        response => response,
        async error => {
            const { response: { status = null } = {} } = error || {}

            switch (status) {
                case 401:
                    abortRequests()
                    await store.dispatch('user/logout')
                    break
                case 404:
                    await router.push('/404')
                    break
                case null:
                    handleNullStatusCode({ store, error })
                    break
                default:
                    defErrStatusHandler({ store, error })
            }
        }
    )
    return http
}

export { authHttp, http, abortRequests, getBaseHttp }
